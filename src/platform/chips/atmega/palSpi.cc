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
 * @author Andreas Weigel
 */

#include "palSpi.h"
#include "palExec.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "palSpiPins.h"

static palSpi_cbPtr cb = NULL;
static uint8_t * rxBuffer = NULL;
static const uint8_t * txBuffer = NULL;
static volatile uint16_t pos;
static uint16_t toSend;
static uint8_t next = 0;
static volatile uint8_t dummy;


inline void palSpi_intEnable() {
	(SPCR |= (1 << SPIE));
}
inline void palSpi_intDisable() {
	(SPCR &= ~(1 << SPIE));
}

static inline bool isMaster() {
	return (SPCR & (1 << MSTR)) != 0;
}

void palSpi_enable() {
	if (isMaster()) {
		SPI_PORT &= ~(1 << SPI_SS_PIN);
	}
}

void palSpi_disable() {
	if (isMaster()) {
		SPI_PORT |= (1 << SPI_SS_PIN);
	}
}


static inline void palSpi_setFrequency(uint8_t prescaler) {
	switch(prescaler) {
		case 2: {
			SPCR &= ~((1 << SPR1) | (1 << SPR0));
			SPSR |= (1 << SPI2X);
			break;
		}
		case 8: {
			SPCR &= ~(1 << SPR1);
			SPCR |= (1 << SPR0);
			SPSR |= (1 << SPI2X);
			break;
		}
		case 32: {
			SPCR &= ~(1 << SPR0);
		    SPCR |= (1 << SPR1);
			SPSR |= (1 << SPI2X);
			break;
		}
		case 4: {
			SPCR &= ~((1 << SPR1) | (1 << SPR0));
			SPSR &= ~(1 << SPI2X);
			break;
		}
		case 16: {
			SPCR &= ~(1 << SPR1);
			SPCR |= (1 << SPR0);
			break;
		}
		case 64: {
			SPCR |= (1 << SPR1);
			SPCR &= ~(1 << SPR0);
			SPSR &= ~(1 << SPI2X);
			break;
		}
		case 128: {
			SPCR |= (1 << SPR1) | (1 << SPR0);
			SPSR &= ~(1 << SPI2X);
			break;
		}
	}
}

cometos_error_t palSpi_init(bool asMaster, uint32_t freq, palSpi_mode_t mode, bool lsbFirst) {
	if (lsbFirst) {
	    SPCR |= (1 << DORD);
	} else {
	    SPCR &= ~(1 << DORD);
	}

    if (mode >= 0 && mode <= 3) {
	    uint8_t cpha = mode & 0x01;
	    uint8_t cpol = mode & 0x02 ? 1 : 0;

	    // first clear both bits, then set them according to given mode
	    SPCR &= ~( (1 << CPHA) | (1 << CPOL) );
	    SPCR |= (cpha << CPHA) | (cpol << CPOL);
	} else {
	    return COMETOS_ERROR_INVALID;
	}

    if (asMaster) {
		bool freqValid = false;
		uint16_t i;
		for (i=2; i <= 128; i *= 2) {
			if (freq * i == F_CPU) {
				freqValid = true;
				break;
			}
		}

		if (!freqValid) {
			return COMETOS_ERROR_INVALID;
		}

		palSpi_setFrequency(i);

		// setup slave select, sck and mosi pins as output
		SPI_DDR |= (1 << SPI_SS_PIN) | (1 << SPI_SCK_PIN) | (1 << SPI_OUT_PIN);
		

	    	// enable SPI as master
	    	SPCR |= (1 << SPE) | (1 << MSTR);

	   	palSpi_disable();
	} else {

		// setup miso as output pin
		SPI_DDR |= (1 << SPI_IN_PIN);

		// enable SPI as slave
		SPCR |= (1 << SPE);
	}

	return COMETOS_SUCCESS;
}


cometos_error_t palSpi_swapByte(uint8_t tx, uint8_t* rx) {
	palSpi_intDisable();
	bool busy = (txBuffer != NULL || rxBuffer != NULL || cb != NULL);
	if (busy) {
		palSpi_intEnable();
		return COMETOS_ERROR_BUSY;
	}

	if ((SPCR & (1 << SPE)) == 0) {
		return COMETOS_ERROR_OFF;
	}

	// start actual transmission
	SPDR = tx;
	while (!(SPSR & (1 << SPIF)))
		;
	(*rx) = SPDR;

	return COMETOS_SUCCESS;
}
cometos_error_t palSpi_transmitBlocking(const uint8_t * txBuf, uint8_t * rxBuf, uint16_t len) {
	palExec_atomicBegin();
	bool busy = (txBuffer != NULL || rxBuffer != NULL || cb != NULL);
	if (busy) {
		palExec_atomicEnd();
		return COMETOS_ERROR_BUSY;
	}

	if (txBuf == NULL && rxBuf == NULL) {
		palExec_atomicEnd();
		return COMETOS_ERROR_INVALID;
	}
	rxBuffer = rxBuf;
	txBuffer = txBuf;
	palExec_atomicEnd();

	for (uint16_t i=0; i < len; i++) {
		SPDR = txBuffer != NULL ? txBuffer[i] : dummy;
		while (!(SPSR & (1 << SPIF)))
				;
		if (rxBuffer != NULL) {
		    rxBuffer[i] = SPDR;
		} else {
		    dummy = SPDR;
		}
#ifdef COMETOS_SPI_ADD_INTER_BYTE_DELAY
		if (isMaster()) {
			// give some time to slave
			_delay_loop_1(50);
		}
#endif
	}

	palExec_atomicBegin();
	txBuffer = NULL;
	rxBuffer = NULL;
	palExec_atomicEnd();

	return COMETOS_SUCCESS;
}

cometos_error_t palSpi_transmit(const uint8_t * txBuf, uint8_t * rxBuf, uint16_t len, palSpi_cbPtr callback) {
	palExec_atomicBegin();
	bool busy = (txBuffer != NULL || rxBuffer != NULL || cb != NULL);
	if (busy) {
		palExec_atomicEnd();
		return COMETOS_ERROR_BUSY;
	}

	if ((txBuf == NULL && rxBuf == NULL) || callback == NULL) {
		palExec_atomicEnd();
		return COMETOS_ERROR_INVALID;
	}

	cb = callback;
	txBuffer = txBuf;
	rxBuffer = rxBuf;

	pos = 0;
	toSend = len;

	// if txBuffer is NULL, send dummy byte instead
	next = txBuffer != NULL ? txBuffer[pos+1] : dummy;

	palSpi_intEnable();
	// just to ensure that the interrupt flag is cleared
	dummy = SPSR;
	dummy = SPDR;
	palExec_atomicEnd();

	// if txBuffer is NULL, send dummy byte instead
	SPDR = txBuffer != NULL ? txBuffer[pos] : dummy;

	return COMETOS_SUCCESS;
}

ISR( SPI_STC_vect) {
#ifdef COMETOS_SPI_ADD_INTER_BYTE_DELAY
	if (isMaster()) {
		// give some time to slave
		_delay_loop_1(50);
	}
#endif

	// read received byte from buffer, if rxBuffer is NULL, store into dummy
	if (rxBuffer != NULL) {
	    rxBuffer[pos] = SPDR;
	} else {
	    dummy = SPDR;
	}

	pos++;
	if (pos == toSend) {
		palSpi_intDisable();
		uint8_t * tmpRxBuf = rxBuffer;
		const uint8_t * tmpTxBuf = txBuffer;
		palSpi_cbPtr tmpCb = cb;
		uint8_t tmpToSend = toSend;
		cb = NULL;
		toSend = 0;
		rxBuffer = NULL;
		txBuffer = NULL;
		tmpCb(tmpTxBuf, tmpRxBuf, tmpToSend, COMETOS_SUCCESS);
	} else {
	    // write next byte to SPI
        SPDR = next;
	    // set next byte or let dummy stay there
		if (txBuffer != NULL) {
		    next = txBuffer[pos+1];
		}
	}
}
