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
 * @author Florian Meier (For ARM)
 */

#include "device/fsl_device_registers.h"

#include <stdlib.h>
#include "palSerial.h"
#include "palLed.h"
#include "Callback.h"

#include "cometos.h"

namespace cometos {

/*MACROS---------------------------------------------------------------------*/

/* Shifts registers for UART1
 * 0xCE-0xC6 = 8
 */
#define REG(x) (*((&x)+0x1000*Peripheral))

#define TX_BUFFER_SIZE	128

#define RX_BUFFER_SIZE	128

template<int Peripheral>
class PalSerialImpl : public PalSerial {
	// assert that REG macro is correct for UART1
	static_assert(Peripheral != 1 || &REG(UART0_C1) == &UART1_C1,"Calculation is wrong!");

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
		REG(UART0_C2) &= ~(UART_C2_RIE_MASK);
	}

	inline void ENABLE_UART_RX_INT() {
		REG(UART0_C2) |= (UART_C2_RIE_MASK);
	}

	inline void DISABLE_UART_TX_FIFO_INT() {
		REG(UART0_C2) &= ~(UART_C2_TIE_MASK);
	}

	inline void ENABLE_UART_TX_FIFO_INT() {
		REG(UART0_C2) |= (UART_C2_TIE_MASK);
	}

	inline void DISABLE_UART_TX_RDY_INT() {
		REG(UART0_C2) &= ~(UART_C2_TCIE_MASK);
	}

	inline void ENABLE_UART_TX_RDY_INT() {
		REG(UART0_C2) |= (UART_C2_TCIE_MASK);
	}

	// pin muxing has to be done outside
	inline void init(uint32_t baudrate, Task* rxStartCallback,
			Task* txFifoEmptyCallback, Task* txReadyCallback) {
		rxPos = 0;
		txPos = 0;
		rxLength = 0;
		txLength = 0;
		using9bit = false;
		skipBytes = false;
		this->txFifoEmptyCallback = txFifoEmptyCallback;
		this->txReadyCallback = txReadyCallback;
		this->rxStartCallback = rxStartCallback;

#if 0
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
		REG(UCSR0B) |= (1 << RXEN0) | (1 << TXEN0);

#endif
		SIM_SCGC4 |= (SIM_SCGC4_UART0_MASK << Peripheral);

		DISABLE_UART_RX_INT();
		DISABLE_UART_TX_FIFO_INT();
		DISABLE_UART_TX_RDY_INT();

		REG(UART0_C2) &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK );
		REG(UART0_C1) = 0;

		// UART baud rate = UART module clock/(Baud rate * 16)
		// UART0 and UART1 are clocked from the CPU clock - all others with fnet_mk_periph_clk_khz();
		uint32_t clock = F_CPU;
		if(Peripheral >= 2) {
			clock = F_CPU / (((SIM_CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> 24) + 1);
		}
		uint16_t ubd1 = (uint16_t)((clock)/(baudrate*16));	/* Calculate baud settings */

		//Set UART_BDH - UART_BDL register
		uint8_t temp1 = REG(UART0_BDH) & ~(UART_BDH_SBR(0x1F));/*Save the value of UART0_BDH except SBR*/

		//concatenate ubd and temp to UARTx_BDH (3 bits from temp + 5 bits from ubd)
		REG(UART0_BDH) = temp1 | (((ubd1 & 0x1F00) >> 8));
		REG(UART0_BDL) = (uint8_t)(ubd1 & UART_BDL_SBR_MASK);

		REG(UART0_C2) |= (UART_C2_TE_MASK | UART_C2_RE_MASK); /* Enable receiver and transmitter*/

		//NVIC_EnableIRQ((IRQn_Type)(UART0_RX_TX_IRQn+8*Peripheral));
		//NVIC_EnableIRQ(UART2_RX_TX_IRQn);
		switch(Peripheral) {
		case 0:
			NVIC_EnableIRQ(UART0_RX_TX_IRQn);
			break;
		case 1:
			NVIC_EnableIRQ(UART1_RX_TX_IRQn);
			break;
		case 2:
			NVIC_EnableIRQ(UART2_RX_TX_IRQn);
			break;
		case 3:
			NVIC_EnableIRQ(UART3_RX_TX_IRQn);
			break;
		}

		ENABLE_UART_RX_INT();
	}

	inline void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback) {
        this->rxByteCallback = rxByteCallback;

	    if(enable) {
            /* Set character size to 9 bit. */
            REG(UART0_C1) |= (UART_C1_M_MASK);
	    }
	    else {
            /* Set character size to 8 bit. */
            REG(UART0_C1) &= ~(UART_C1_M_MASK);
	    }

        skipBytes = using9bit = (enable && multiProcessorMode);
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
		if (txLength == 0) {
			if(txReadyCallback != NULL) {
				txReadyCallback->invoke();
			}

			DISABLE_UART_TX_RDY_INT();
		}
	}

	inline void txFifoISR() {
		if (txLength > 0) {
			if(using9bit) {
				if(txStartFrameFlag[txPos]) {
					REG(UART0_C3) |= (UART_C3_T8_MASK);
				}
				else {
					REG(UART0_C3) &= ~(UART_C3_T8_MASK);
				}
			}

			REG(UART0_D) = txBuffer[txPos];
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
		bool nineth = ((REG(UART0_C3) & (UART_C3_R8_MASK)) != 0);
		uint8_t b = REG(UART0_D);

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
					//REG(UCSR0A) |= (1 << MPCM0);

					skipBytes = true;
				}
				else {
					/* Disable Multi-Processor Mode (do not discard bytes) */
					//REG(UCSR0A) &= ~(1 << MPCM0);
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

	void isr() {
		if((REG(UART0_S1) & UART_S1_TDRE_MASK) && (REG(UART0_C2) & UART_C2_TIE_MASK)) {
			txFifoISR();
		}

		if((REG(UART0_S1) & UART_S1_TC_MASK) && (REG(UART0_C2) & UART_C2_TCIE_MASK) && txLength == 0) {
			txReadyISR();
		}

		if((REG(UART0_S1) & UART_S1_RDRF_MASK) && (REG(UART0_C2) & UART_C2_RIE_MASK)) {
			rxISR();
		}

		//NVIC_ClearPendingIRQ((IRQn_Type)(UART0_RX_TX_IRQn+8*Peripheral));
	}

	static PalSerialImpl& getInstance() {
		// Instantiate class
		static PalSerialImpl<Peripheral> uart;
		return uart;
	}
};

template<> PalSerial* PalSerial::getInstance<int>(int peripheral) {
	switch(peripheral) {
	case 0:
		return &PalSerialImpl<0>::getInstance();
	case 1:
		return &PalSerialImpl<1>::getInstance();
	case 2:
		return &PalSerialImpl<2>::getInstance();
	case 3:
		return &PalSerialImpl<3>::getInstance();
	}
	return NULL;
}

// Interrupt handlers
extern "C" {
	void UART0_RX_TX_IRQHandler(void)
	{
		PalSerialImpl<0>::getInstance().isr();
	}

	void UART1_RX_TX_IRQHandler(void)
	{
		PalSerialImpl<1>::getInstance().isr();
	}

	void UART2_RX_TX_IRQHandler(void)
	{
		PalSerialImpl<2>::getInstance().isr();
	}

	void UART3_RX_TX_IRQHandler(void)
	{
		PalSerialImpl<3>::getInstance().isr();
	}
}

}


