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

/** EEPROM TWI DRIVER FOR ATMEL'S AT24C1024B DEVICE
 *
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "twi_eeprom.h"
#include "twi.h"
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
 * �s.  The longest write period is supposed to not exceed ~ 10 ms.
 * Thus, normal operation should not require more than 100 iterations
 * to get the device to respond to a selection.
 */
#define MAX_ITER	200

/*FUNCTION DEFINITION--------------------------------------------------------*/

/**Writes data to EEPROM. Not that addressing is done
 * with 16-Bit word addresses.
 *
 * @addr_page 	page address
 * @offset		8-bit offset in page
 * @data		pointer to data
 * @length  	number of byte (8-Bit) to write
 * @return		number of written bytes
 */
uint16_t ext_eeprom_writeInPage(uint32_t addr, const uint8_t *data, uint16_t length) {

	uint8_t status;
	uint16_t nWritten = 0;
	uint16_t iter;

	// get device address and shift MSB bit to position 2 of device address
	uint8_t device_address = TWI_EEPROM_ADDRESS | ((addr >> 15) & 0x02);

	// check for valid address
	if ((addr / EEPROM_PAGE_SIZE) > EEPROM_NUM_PAGES) {
		return nWritten;
	}

	// check for valid end address
	if ((addr + length) / EEPROM_PAGE_SIZE > EEPROM_NUM_PAGES) {
		return nWritten;
	}

	// check for space left in current page
	if (length > EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE)) {
		length = EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE);
	}

	// loop is needed if device is busy
	for (iter = 0; iter < MAX_ITER; iter++) {

		// start
		status = twi_start();

		if (status != TW_START && status != TW_REP_START) {
//			printf("A\n");
			twi_stop();
			return nWritten;
		}


		// select device for writing
		status = twi_send(device_address | TW_WRITE);

		if (TW_MT_SLA_NACK == status ) {
			continue;
		} else if (TW_MT_SLA_ACK == status) {
			break;
		} else {
//			printf("B %x\n", status);
			twi_stop();
			return nWritten;
		}
	}

	if (iter == MAX_ITER) {
//		printf("B2\n");
		return nWritten;
	}

	// send address byte 1
	status = twi_send(addr >> 8);
	if (TW_MT_DATA_ACK != status) {
//		printf("C\n");
		twi_stop();
		return nWritten;
	}

	// send address byte 2
	status = twi_send(addr);
	if (TW_MT_DATA_ACK != status) {
//		printf("D\n");
		twi_stop();
		return nWritten;
	}

	// write data
	for (; length > 0; length--) {
		status = twi_send(*data);
		data++;
		if (status != TW_MT_DATA_ACK) {
			break;
		}
		nWritten++;
	}

	twi_stop();
	return nWritten;

}

void ext_eeprom_init() {
	twi_init();
	DDRD |= (1 << 6);
	PORTD &= ~(1 << 6);
}

uint16_t ext_eeprom_write(uint32_t addr, const uint8_t *data, uint16_t length) {

	uint16_t nWritten = 0;
	uint16_t nTemp;

	while (length) {
		nTemp = ext_eeprom_writeInPage(addr, data, length);
		if (nTemp == 0) {
			break;
		}
		nWritten += nTemp;
		addr += nTemp;
		data += nTemp;
		length -= nTemp;
	}

	return nWritten;

}

uint16_t ext_eeprom_read(uint32_t addr, uint8_t *data, uint16_t length) {

	uint8_t status;
	uint16_t nRead = 0;
	uint16_t iter;

	// get device address and shift MSB bit to position 2 of device address
	uint8_t device_address = TWI_EEPROM_ADDRESS | ((addr >> 15) & 0x02);

	// check for valid address
	if ((addr / EEPROM_PAGE_SIZE) > EEPROM_NUM_PAGES) {
		return nRead;
	}

	// check for valid end address
	if ((addr + length) / EEPROM_PAGE_SIZE > EEPROM_NUM_PAGES) {
		return nRead;
	}


	// loop is needed if device is busy
	for (iter = 0; iter < MAX_ITER; iter++) {

		// start
		status = twi_start();

		if (status != TW_START && status != TW_REP_START) {
//			printf("A\n");
			twi_stop();
			return nRead;
		}


		// select device for writing
		status = twi_send(device_address | TW_WRITE);

		if (TW_MT_SLA_NACK == status ) {
			continue;
		} else if (TW_MT_SLA_ACK == status) {
			break;
		} else {
//			printf("B %x\n", status);
			twi_stop();
			return nRead;
		}
	}

	if (iter == MAX_ITER) {
//		printf("B2\n");
		return nRead;
	}

	// send address byte 2
	status = twi_send(addr >> 8);
	if (TW_MT_DATA_ACK != status) {
		twi_stop();
		return nRead;
	}

	// send address byte 1
	status = twi_send(addr);
	if (TW_MT_DATA_ACK != status) {
		twi_stop();
		return nRead;
	}

	// send start again
	status = twi_start();
	if (status != TW_REP_START) {
		twi_stop();
		return nRead;
	}

	// select device for reading
	status = twi_send(device_address | TW_READ);
	if (TW_MR_SLA_ACK != status) {
		twi_stop();
		return nRead;
	}

	// read data
	for (; length > 0; length--) {
		if (length == 1) {
			status = twi_receive(data, false);
		} else {
			status = twi_receive(data, true);
		}
		data++;
		if (status != TW_MR_DATA_ACK) {
			if (length == 1) {
				nRead++;
			}
			break;
		}
		nRead++;
	}

	twi_stop();

	return nRead;
}
