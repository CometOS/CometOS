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
 * @author Stefan Unterschuetz
 * @author Florian Meier (Generic for UART0 and UART1)
 */

#include <avr/io.h>
#include <stdlib.h>
#include "palSerial.h"
#include <avr/interrupt.h>
#include "palLed.h"
#include "Callback.h"

#include "cometos.h"

using namespace cometos;

/*MACROS---------------------------------------------------------------------*/

/* Shifts registers for UART1
 * 0xCE-0xC6 = 8
 */
#define REG(x) (*((&x)+8*Peripheral))

#ifndef SERIAL_BUFFER_SIZE
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128
#else
#define TX_BUFFER_SIZE SERIAL_BUFFER_SIZE
#define RX_BUFFER_SIZE SERIAL_BUFFER_SIZE
#endif

/*
 * Calculate the UART baud rate generator prescaler, based on the
 * global F_CPU setting, and the baud rate passed as parameter.  This
 * assumes the U2X bit will always be set.
 */
#define UART_BAUD(rate) \
    (((F_CPU) + 4UL * (rate)) / (8UL * (rate)) - 1UL)

namespace cometos {

template<int Peripheral>
class PalSerialImpl : public PalSerial {
	// assert that REG macro is correct for UART1
#ifdef UBRR1H
	static_assert(Peripheral != 1 || &REG(UBRR0H) == &UBRR1H,"Calculation is wrong!");
#endif

/*VARIABLES------------------------------------------------------------------*/
private:
	uint8_t rxBuffer[RX_BUFFER_SIZE];
	uint8_t txBuffer[TX_BUFFER_SIZE];
	uint8_t txStartFrameFlag[TX_BUFFER_SIZE];

	volatile uint8_t rxLength;
	volatile uint8_t txLength;

	volatile uint8_t rxPos;
	volatile uint8_t txPos;

	bool using9bit;
	bool skipBytes;

	Task* txFifoEmptyCallback;
	Task* txReadyCallback;
	Task* rxStartCallback;
	Callback<bool(uint8_t,bool)> rxByteCallback;

/*FUNCTION DEFINITION--------------------------------------------------------*/
public:
	inline void DISABLE_UART_RX_INT() {
		REG(UCSR0B) &= ~(1 << RXCIE0);
	}

	inline void ENABLE_UART_RX_INT() {
		REG(UCSR0B) |= (1 << RXCIE0);
	}

	inline void DISABLE_UART_TX_FIFO_INT() {
		REG(UCSR0B) &= ~(1 << UDRIE0);
	}

	inline void ENABLE_UART_TX_FIFO_INT() {
		REG(UCSR0B) |= (1 << UDRIE0);
	}

	inline void DISABLE_UART_TX_RDY_INT() {
		REG(UCSR0B) &= ~(1 << TXCIE0);
	}

	inline void ENABLE_UART_TX_RDY_INT() {
		REG(UCSR0B) |= (1 << TXCIE0);
	}


	inline void init(uint32_t baudrate, Task* rxStartCallback,
			Task* txFifoEmptyCallback, Task* txReadyCallback) {
		DISABLE_UART_RX_INT();
		DISABLE_UART_TX_FIFO_INT();
		DISABLE_UART_TX_RDY_INT();


		rxPos = 0;
		txPos = 0;
		rxLength = 0;
		txLength = 0;
		using9bit = false;
		skipBytes = false;
		this->txFifoEmptyCallback = txFifoEmptyCallback;
		this->txReadyCallback = txReadyCallback;
		this->rxStartCallback = rxStartCallback;

		/* Calculate corresponding value for baud rateregister. */
		uint16_t baud_rate_reg = UART_BAUD(baudrate);

		/*
		 * Microcontroller's USART register is updated to
		 * run at the given baud rate.
		 */
		REG(UBRR0H) = (baud_rate_reg >> 8) & 0xFF;
		REG(UBRR0L) = (uint8_t) baud_rate_reg;

		/* Faster async mode (UART clock divider = 8, instead of 16) */
		REG(UCSR0A) = (1 << U2X0);

		/* Data Length is 8 bit */
		REG(UCSR0C) = (1 << UCSZ01) | (1 << UCSZ00);

		/*
		 * Receiver and transmitter are enabled.
		 * Receive and transmit interrrupt are enabled.
		 */
		REG(UCSR0B) = (1 << RXEN0) | (1 << TXEN0);

		ENABLE_UART_RX_INT();
	}

	inline void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback) {
        this->rxByteCallback = rxByteCallback;

	    if(enable) {
            /* Set character size to 9 bit. */
            REG(UCSR0B) |= (1 << UCSZ02);
            REG(UCSR0C) |= (1 << UCSZ01) | (1 << UCSZ00);
	    }
	    else {
            /* Set character size to 8 bit. */
            REG(UCSR0B) &= ~(1 << UCSZ02);
            REG(UCSR0C) |= (1 << UCSZ01) | (1 << UCSZ00);
	    }

        if(enable && multiProcessorMode) {
            /* Enable Multi-Processor Mode (discard bytes until the next flagged byte) */
            REG(UCSR0A) |= (1 << MPCM0);

            skipBytes = true;
            using9bit = true;
        }
        else {
            /* Disable Multi-Processor Mode */
            REG(UCSR0A) &= ~(1 << MPCM0);

            skipBytes = false;
            using9bit = false;
        }
	}

	inline uint8_t write(const uint8_t* data, uint8_t length, bool flagFirst = true) {
		DISABLE_UART_TX_RDY_INT();
		DISABLE_UART_TX_FIFO_INT();
		uint8_t len = txLength;
		uint8_t pos = txPos;
		ENABLE_UART_TX_FIFO_INT();

		// get maximum number of bytes that can be written
		if ((TX_BUFFER_SIZE - len) < length) {
			length = (TX_BUFFER_SIZE - len);
		}

		// get start position for writing
		pos = (pos + len) % TX_BUFFER_SIZE;

		for (uint8_t i = 0; i < length; i++) {
			txBuffer[pos] = data[i];
			
			if(i == 0 && flagFirst) {
				txStartFrameFlag[pos] = 1;
			}
			else {
				txStartFrameFlag[pos] = 0;
			}

			pos = (pos + 1) % TX_BUFFER_SIZE;
		}

		DISABLE_UART_TX_FIFO_INT();
		txLength += length;
		ENABLE_UART_TX_FIFO_INT();
		ENABLE_UART_TX_RDY_INT();

		return length;
	}

	inline uint8_t read(uint8_t* data, uint8_t length) {
		DISABLE_UART_RX_INT();
		uint8_t len = rxLength;
		uint8_t pos = rxPos;
		ENABLE_UART_RX_INT();

		// get maximum number of bytes that can be read
		if (len < length) {
			length = len;
		}

		// get start position for reading
		pos = (RX_BUFFER_SIZE + pos - len) % RX_BUFFER_SIZE;

		for (uint8_t i = 0; i < length; i++) {
			data[i] = rxBuffer[pos];
			pos = (pos + 1) % RX_BUFFER_SIZE;
		}

		DISABLE_UART_RX_INT();
		rxLength -= length;
		ENABLE_UART_RX_INT();

		return length;
	}

	inline void txReadyISR() {
		if (txLength == 0 && txReadyCallback != NULL) {
			txReadyCallback->invoke();
			DISABLE_UART_TX_RDY_INT();
		}
	}

	inline void txFifoISR() {
		if (txLength > 0) {
			if(using9bit) {
				if(txStartFrameFlag[txPos]) {
					REG(UCSR0B) |= (1 << TXB80);
				}
				else {
					REG(UCSR0B) &= ~(1 << TXB80);
				}
			}

			REG(UDR0) = txBuffer[txPos];
			txLength--;
			txPos = (txPos + 1) % TX_BUFFER_SIZE;
			if (txLength == 0 && txFifoEmptyCallback != NULL) {
				txFifoEmptyCallback->invoke();
			}
		} else {
			DISABLE_UART_TX_FIFO_INT();
		}
	}

	inline void rxISR() {
		bool nineth = ((REG(UCSR0B) & (1 << RXB80)) != 0);
		uint8_t b = REG(UDR0);

		if(using9bit) {
		       	if(skipBytes && !nineth) {
				/* Skip this byte (this is done by the Multi-Processor Mode, too,
				 * but this is not guaranteed for bytes in the buffer or when
				 * setting the Multi-Processor Mode late)
				 */
				return;
			}
			else {
				skipBytes = false;
			}

			if(rxByteCallback) {
				bool cont = rxByteCallback(b,nineth);

				if(!cont) {
					/* Enable Multi-Processor Mode (discard bytes until the next flagged byte) */
					REG(UCSR0A) |= (1 << MPCM0);

					skipBytes = true;
				}
				else {
					/* Disable Multi-Processor Mode (do not discard bytes) */
					REG(UCSR0A) &= ~(1 << MPCM0);
				}
			}
		}

		if (rxLength < RX_BUFFER_SIZE) {
			rxBuffer[rxPos] =b;
			rxPos = (rxPos + 1) % RX_BUFFER_SIZE;
			rxLength++;
			if (rxLength == 1 && rxStartCallback != NULL) {
				rxStartCallback->invoke();
			}
		}
	}

	static PalSerialImpl& getInstance() {
		// Instantiate class
		static PalSerialImpl<Peripheral> uart;
		return uart;
	}
};

template<> PalSerial* PalSerial::getInstance<int>(int peripheral) {
	if(peripheral == 0) {
		return &PalSerialImpl<0>::getInstance();
	}
	else {
		return &PalSerialImpl<1>::getInstance();
	}
}

}

// Interrupt handlers
#ifdef USART0_UDRE_vect
ISR( USART0_UDRE_vect) {
	PalSerialImpl<0>::getInstance().txFifoISR();
}

ISR( USART0_RX_vect) {
	PalSerialImpl<0>::getInstance().rxISR();
}

ISR( USART0_TX_vect) {
	PalSerialImpl<0>::getInstance().txReadyISR();
}
#else
ISR( USART_UDRE_vect) {
	PalSerialImpl<0>::getInstance().txFifoISR();
}

ISR( USART_RX_vect) {
	PalSerialImpl<0>::getInstance().rxISR();
}

ISR( USART_TX_vect) {
	PalSerialImpl<0>::getInstance().txReadyISR();
}
#endif

#ifdef USART1_UDRE_vect

ISR( USART1_UDRE_vect) {
	PalSerialImpl<1>::getInstance().txFifoISR();
}

ISR( USART1_RX_vect) {
	PalSerialImpl<1>::getInstance().rxISR();
}

ISR( USART1_TX_vect) {
	PalSerialImpl<1>::getInstance().txReadyISR();
}

#endif
