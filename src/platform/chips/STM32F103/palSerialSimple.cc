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

#include "palSerial.h"
#include "palExec.h"
#include "cometos.h"
#include "timer2.h"
#include "cmsis_device.h"

#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"


#define TX_BUFFER_SIZE	128
#define RX_BUFFER_SIZE	128

namespace cometos {

template<int Peripheral>
class PalSerialImpl : public PalSerial {

private:
	uint8_t rxBuffer[RX_BUFFER_SIZE];
	uint8_t txBuffer[TX_BUFFER_SIZE];

	Task *txFifoEmptyCallback, *txReadyCallback, *rxStartCallback;
	Callback<bool(uint8_t, bool)> rxByteCallback;

	volatile uint8_t rxLength;
	volatile uint8_t txLength;

	volatile uint8_t rxPos;
	volatile uint8_t txPos;


	bool using9bit;
	bool skipBytes;

	USART_TypeDef* usart  = 0;

public:
	void init(uint32_t baudrate, Task* rxStartCallback, Task* txFifoEmptyCallback, Task* txReadyCallback){

		switch (Peripheral){
		case 1:
			usart = USART1;
			break;
		case 2:
			usart = USART2;
			break;
		case 3:
			usart = USART3;
			break;
		case 4:
			usart = UART4;
			break;
		case 5:
			usart = UART5;
			break;
		}
		USART_Cmd(usart, DISABLE);
		USART_DeInit(usart);


		// init local variables
		rxPos = 0;
		txPos = 0;
		rxLength = 0;
		txLength = 0;
		using9bit = false;
		skipBytes = false;

		this->txFifoEmptyCallback = txFifoEmptyCallback;
		this->txReadyCallback = txReadyCallback;
		this->rxStartCallback = rxStartCallback;

		// Enable interrupt line and clock
		enableInterruptLine(Peripheral);

		// Init UART
		USART_InitTypeDef USART_InitStructure;

		USART_InitStructure.USART_BaudRate = baudrate;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init(usart, &USART_InitStructure);
//		USART_DMACmd(usart, USART_DMAReq_Tx, DISABLE);

		// Enable Uart
		ENABLE_UART_RX_INT();
		USART_Cmd(usart, ENABLE);
	}

	inline void enableInterruptLine(int line)
	{
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0f;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0f;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

		switch (line){
		case 1:
			NVIC_EnableIRQ(USART1_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE );
			break;
		case 2:
			NVIC_EnableIRQ(USART2_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE );
			break;
		case 3:
			NVIC_EnableIRQ(USART3_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE );
			break;
		case 4:
			NVIC_EnableIRQ(UART4_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE );

			break;
		case 5:
			NVIC_EnableIRQ(UART5_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE );
			break;
		default:
			//TODO: Throw ERROR
			break;
		}


		NVIC_Init(&NVIC_InitStructure);
	}

	// UART received byte interrupt
	inline void DISABLE_UART_RX_INT() {
		usart->CR1 &= ~USART_CR1_RXNEIE;
	}

	inline void ENABLE_UART_RX_INT() {
		usart->CR1 |= USART_CR1_RXNEIE;
	}

	// UART Transmission FIFO Empty Interrupt
	inline void DISABLE_UART_TX_FIFO_INT() {
		usart->CR1 &= ~USART_CR1_TXEIE;
	}

	inline void ENABLE_UART_TX_FIFO_INT() {
		usart->CR1 |= USART_CR1_TXEIE;
	}

	//UART Clear to send Interrupt
	inline void DISABLE_UART_TX_RDY_INT() {
		usart->CR1 &= ~USART_CR1_TCIE;
	}

	inline void ENABLE_UART_TX_RDY_INT() {
		usart->CR1 |= USART_CR1_TCIE;
	}

	//TODO: Implement 9bit mode
	inline void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback){
		using9bit = true;
	}

	inline uint8_t write(const uint8_t* data, uint8_t length, bool flagFirst = true){
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

            pos = (pos + 1) % TX_BUFFER_SIZE;
        }

        DISABLE_UART_TX_FIFO_INT();
        txLength += length;
        ENABLE_UART_TX_FIFO_INT();
        ENABLE_UART_TX_RDY_INT();

        return length;
	}


	inline uint8_t read(uint8_t* data, uint8_t length)
	{
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


	void txReadyISR() {
		if (txLength == 0) {
			if(txReadyCallback != NULL) {
				txReadyCallback->invoke();
				DISABLE_UART_TX_RDY_INT();
			}
		}
		usart->SR &= ~(USART_SR_TC);
	}

	void txFifoISR() {
		if (txLength > 0) {
			/*if(using9bit) {
				if(txStartFrameFlag[txPos]) {
					REG(UART0_C3) |= (UART_C3_T8_MASK);
				}
				else {
					REG(UART0_C3) &= ~(UART_C3_T8_MASK);
				}
			}*/

		    //usart->DR = 0xA0 & (uint16_t)0x01FF;
		    usart->DR = txBuffer[txPos];
			txLength--;
			txPos = (txPos + 1) % TX_BUFFER_SIZE;

			if (txLength == 0 && txFifoEmptyCallback != NULL) {
				txFifoEmptyCallback->invoke();
			}
		} else {
		    DISABLE_UART_TX_FIFO_INT();
		}

	}

	void rxISR() {
		palExec_atomicBegin();
		rxBuffer[rxPos] = (uint8_t) (usart->DR & 0xFF);
		rxPos = (rxPos + 1) % RX_BUFFER_SIZE;

		if (rxLength < RX_BUFFER_SIZE) {
			rxLength++;
		}
		if (rxLength == 1 && rxStartCallback != NULL) {
			rxStartCallback->invoke();
		}
		palExec_atomicEnd();
	}

	void isr(){

		if (usart->CR1 & USART_CR1_RXNEIE)
		{
			if ((usart->SR & USART_SR_RXNE) || (usart->SR & USART_SR_ORE)) {
					rxISR();
			}
		}

		if ((usart->CR1 & USART_CR1_TXEIE) && (usart->SR & USART_SR_TXE )) {
			txFifoISR();
		}

		if ((usart->CR1 & USART_CR1_TCIE) && (usart->SR & USART_SR_TC )) {
			txReadyISR();
		}

	}

	static PalSerialImpl& getInstance() {
		static PalSerialImpl<Peripheral> uart;
		return uart;
	}


protected:
	PalSerialImpl() : usart(NULL) {}

};

template<> PalSerial* PalSerial::getInstance<int>(int peripheral) {
	switch (peripheral){
	case 1:
		return &PalSerialImpl<1>::getInstance();
	case 2:
		return &PalSerialImpl<2>::getInstance();
	case 3:
		return &PalSerialImpl<3>::getInstance();
	default:
		return NULL;
	}
}

// Interrupt handlers
//Interrupt handler declaration in C/C++
#ifdef __cplusplus
extern "C" {
#endif
void USART1_IRQHandler();
void USART2_IRQHandler();
void USART3_IRQHandler();
void USART4_IRQHandler();
void USART5_IRQHandler();

#ifdef __cplusplus
}
#endif


void USART1_IRQHandler(void)
{
	PalSerialImpl<1>::getInstance().isr();
}

void USART2_IRQHandler(void)
{
	PalSerialImpl<2>::getInstance().isr();
}

void USART3_IRQHandler(void)
{
	PalSerialImpl<3>::getInstance().isr();
}

void UART4_IRQHandler(void)
{
	PalSerialImpl<4>::getInstance().isr();
}

void UART5_IRQHandler(void)
{
	PalSerialImpl<5>::getInstance().isr();
}

}



