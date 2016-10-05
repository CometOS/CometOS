/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * EEPROM TWI DRIVER FOR ATMEL'S AT24C1024B DEVICE
 *
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "eepromAsync.h"
#include "twiAsync.h"
#include <avr/io.h>
#include <stdio.h>


/*MACROS---------------------------------------------------------------------*/

/**TWI address of eeprom*/
#define TWI_EEPROM_ADDRESS	0xA0

/**number of pages*/
#define EEPROM_NUM_PAGES	512

/**page size*/
#define EEPROM_PAGE_SIZE	256

/** (doc from external source)
 * Maximal number of iterations to wait for a device to respond for a
 * selection.  Should be large enough to allow for a pending write to
 * complete, but low enough to properly abort an infinite loop in case
 * a slave is broken or not present at all.  With 100 kHz TWI clock,
 * transfering the start condition and SLA+R/W packet takes about 10
 * us.  The longest write period is supposed to not exceed ~ 10 ms.
 * Thus, normal operation should not require more than 100 iterations
 * to get the device to respond to a selection.
 */
#define MAX_ITER	200

#define WRITING				3
#define READING				2
#define IDLE				1
#define UNINITIALIZED		0

#define STOP_WITH_ERROR()  state = IDLE; if (currCallback) currCallback(false);return;

typedef void (*state_handler_t)( uint8_t, uint8_t);

/*VARIABLES------------------------------------------------------------------*/
static uint8_t state = UNINITIALIZED;
static state_handler_t handler = NULL;
static uint32_t currAddr = 0;
static uint16_t currLength = 0;
static eeprom_callback_t currCallback;
static uint8_t *currData;
static uint16_t retryCounter = 0;

/*FUNCTION DEFINITION--------------------------------------------------------*/

static void state_startDone(uint8_t twdr, uint8_t status);

void static state_wroteDataByte(uint8_t twdr, uint8_t status) {

	if (status != TW_MT_DATA_ACK) {
		STOP_WITH_ERROR()
	}
	currAddr++;
	currData++;
	currLength--;

	if (currLength == 0) {
		twiAsync_stop();
		state = IDLE;
		if (currCallback)
			currCallback(true);
		return;
	} else if ((currAddr % EEPROM_PAGE_SIZE) == 0) {
		// load new address
		twiAsync_stop();
		state = IDLE;
		retryCounter = 0;
		handler = state_startDone;
		twiAsync_start();
		//eepromAsync_write(currAddr, currData, currLength,
		//	currCallback);
		return;
	} else {
		handler = state_wroteDataByte;
		twiAsync_send(*currData);
	}
}

static void readingReceived(uint8_t twdr, uint8_t status) {

	if (status != TW_MR_DATA_ACK) {
		if (currLength == 1) {
			*currData = twdr;
			currData++;
			handler = NULL;
			state = IDLE;
			twiAsync_stop();
			if (currCallback)
				currCallback(true);
			return;
		} else {
			STOP_WITH_ERROR();
		}
	}

	*currData = twdr;
	currData++;

	currLength--;

	if (currLength == 0) {
		STOP_WITH_ERROR();
	} else if (currLength == 1) {
		twiAsync_receive(false);
	} else {
		twiAsync_receive(true);
	}

}

static void readingSecondSelectDone(uint8_t twdr, uint8_t status) {

	if (TW_MR_SLA_ACK != status) {
		STOP_WITH_ERROR();
	}

	handler = readingReceived;
	if (currLength == 1) {
		twiAsync_receive(false);
	} else {
		twiAsync_receive(true);
	}

	// START READING DATA
}

static void readingSecondStartDone(uint8_t twdr, uint8_t status) {

	if (status != TW_REP_START) {
		STOP_WITH_ERROR();
	}

	handler = readingSecondSelectDone;
	uint8_t device_address = TWI_EEPROM_ADDRESS | ((currAddr >> 15) & 0x02);

	twiAsync_send(device_address | TW_READ);

}

void static state_wroteSecondAddressByte(uint8_t twdr, uint8_t status) {

	if (TW_MT_DATA_ACK != status) {
		STOP_WITH_ERROR();
	}

	if (state == READING) {
		handler = readingSecondStartDone;
		twiAsync_start();
	} else {
		handler = state_wroteDataByte;
		twiAsync_send(*currData);
	}

}

void static state_wroteFirstAddressByte(uint8_t twdr, uint8_t status) {

	if (TW_MT_DATA_ACK != status) {
		STOP_WITH_ERROR();
	}
	handler = state_wroteSecondAddressByte;
	twiAsync_send(currAddr);
}

void static state_selectedForWriting(uint8_t twdr, uint8_t status) {
	retryCounter++;

	if (retryCounter > MAX_ITER) {
		status = 0xff;
		STOP_WITH_ERROR();
		return;
	}
	if (TW_MT_SLA_NACK == status) {
		handler = state_startDone;
		twiAsync_start();
	} else if (TW_MT_SLA_ACK == status) {
		handler = state_wroteFirstAddressByte;
		twiAsync_send(currAddr >> 8);
	} else {
		STOP_WITH_ERROR();
		return;
	}
}

static void state_startDone(uint8_t twdr, uint8_t status) {

	if (status != TW_START && status != TW_REP_START) {
		STOP_WITH_ERROR();
	}

	uint8_t device_address = TWI_EEPROM_ADDRESS | ((currAddr >> 15) & 0x02);
	handler = state_selectedForWriting;
	twiAsync_send(device_address | TW_WRITE);

}

void twiAsync_doneCallback(uint8_t twdr, uint8_t status) {
	if (handler) {
		handler(twdr, status);
	}
}

void eepromAsync_init() {
	twiAsync_init();
	DDRD |= (1 << 6);
	PORTD &= ~(1 << 6);
	if (state == UNINITIALIZED) {
		state = IDLE;
	}
}

bool eepromAsync_read(uint32_t addr, uint8_t* data, uint16_t length,
		eeprom_callback_t callback) {

	// check for valid address
	if ((addr / EEPROM_PAGE_SIZE) > EEPROM_NUM_PAGES) {
		return false;
	}

	// check for valid end address
	if ((addr + length) / EEPROM_PAGE_SIZE > EEPROM_NUM_PAGES) {
		return false;
	}

	if (state != IDLE) {
		return false;
	}

	retryCounter = 0;
	state = READING;
	handler = state_startDone;
	currAddr = addr;
	currLength = length;
	currData = data;
	currCallback = callback;
	twiAsync_start();

	return true;
}

bool eepromAsync_write(uint32_t addr, const uint8_t* data, uint16_t length,
		eeprom_callback_t callback) {
	// check for valid address
	if ((addr / EEPROM_PAGE_SIZE) > EEPROM_NUM_PAGES) {
		return false;
	}

	// check for valid end address
	if ((addr + length) / EEPROM_PAGE_SIZE > EEPROM_NUM_PAGES) {
		return false;
	}

	if (state != IDLE) {
		return false;
	}

	retryCounter = 0;
	state = WRITING;
	handler = state_startDone;
	currAddr = addr;
	currLength = length;
	currData = (uint8_t*)data;
	currCallback = callback;
	twiAsync_start();

	return true;
}
