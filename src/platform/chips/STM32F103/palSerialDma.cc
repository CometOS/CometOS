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
#include "palTimer.h"
#include "cmsis_device.h"

#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

//---------------- DEFINES ------------------------
#define TX_BUFFER_SIZE	128
#define RX_BUFFER_SIZE	128

//Holds which serial ports that should be flushed when timer expires
static bool flush[6];

//---------------- FORWARD DECLARATION ------------
void timerFire();


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

	volatile bool dma_active;
	volatile bool dma_wrapAround;
	volatile uint8_t dma_wrapLength;

	uint32_t dma_tc_flag;
	PalTimer* timer;

	bool using9bit; // TODO: UNUSED
	bool skipBytes; // TODO: UNUSED

	DMA_Channel_TypeDef* txDmaChannel = 0;
	USART_TypeDef* usart  = 0;

public:

	/**
	 *	Initializes the USART.
	 */
	void init(uint32_t baudrate, Task* rxStartCallback, Task* txFifoEmptyCallback, Task* txReadyCallback){

		switch (Peripheral){
		case 1:
			usart = USART1;
			txDmaChannel = DMA1_Channel4;
			dma_tc_flag = DMA1_IT_TC4;
			break;
		case 2:
			usart = USART2;
			txDmaChannel = DMA1_Channel7;
			dma_tc_flag = DMA1_IT_TC7;
			break;
		case 3:
			usart = USART3;
			txDmaChannel = DMA1_Channel2;
			dma_tc_flag = DMA1_IT_TC2;
			break;
		case 4:
			usart = UART4;
			txDmaChannel = DMA2_Channel5;
			dma_tc_flag = DMA2_IT_TC5;
			break;
		case 5:
			usart = UART5;
			txDmaChannel = 0;
			break;
		}
		USART_Cmd(usart, DISABLE);
		USART_DeInit(usart);

		timer = PalTimer::getInstance(Timer::UART);
		timer->init(1e3); //ms

		// init local variables
		rxPos = 0;
		txPos = 0;
		rxLength = 0;
		txLength = 0;
		using9bit = false;
		skipBytes = false;
		dma_active = false;
		dma_wrapAround = false;

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
		USART_DMACmd(usart, USART_DMAReq_Tx, ENABLE);

		// Enable Uart
		ENABLE_UART_RX_INT();
		USART_Cmd(usart, ENABLE);
	}

	/**
	 * Enables the correct DMA interrupt depending on the
	 * usart-port (every usart can only work together with one specific dma
	 * controller).
	 */
	inline void enableInterruptLine(int line)
	{
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0f;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0f;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

		switch (line){
		case 1:
			NVIC_EnableIRQ(USART1_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE );
			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
			break;
		case 2:
			NVIC_EnableIRQ(USART2_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE );
			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
			break;
		case 3:
			NVIC_EnableIRQ(USART3_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE );
			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
			break;
		case 4:
			NVIC_EnableIRQ(UART4_IRQn);
			NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE );
			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

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

	/**
	 * Configures a dma controller to copy byted from a buffer into the
	 * usart data register.
	 *
	 * @param mem_base_address The base address of the buffer to copy from
	 * @param bufferSize The number of bytes to copy from the buffer
	 */
	void configureDMA(uint32_t mem_base_address, uint16_t bufferSize)
	{
		DMA_InitTypeDef DMA_InitStructure;
		DMA_DeInit(txDmaChannel);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(usart->DR);
		DMA_InitStructure.DMA_MemoryBaseAddr = mem_base_address;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_BufferSize = (uint32_t) bufferSize;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

		DMA_Init(txDmaChannel, &DMA_InitStructure);
	}

	//------------- Convenience functions to enable/disable interrupts ------------
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

	/**
	 * TODO: Not implemented yet
	 */
	inline void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback){
		using9bit = true;
	}

	/**
	 *	Copies bytes from the input buffer into the usart buffer. The bytes are not send directly,
	 *	but after a little timeout, or if the usart buffer is filled at least half way. This is due to
	 *	avoid starting dma transfers for single bytes.
	 *
	 *	@param data Base address of input buffer
	 *	@param length number of bytes to copy
	 *	@param flagFirst TODO: Not used till now
	 */
	inline uint8_t write(const uint8_t* data, uint8_t length, bool flagFirst = true){
		flush[Peripheral] = false;

		palExec_atomicBegin();
		uint8_t pos = txPos;
		uint8_t len = txLength;
		palExec_atomicEnd();

		uint8_t characters = 0;
		uint8_t maxLength = TX_BUFFER_SIZE - len;

		if (length > maxLength) {
			length = maxLength;
		}

		for (characters = 0; characters < length; characters++)
		{
			txBuffer[(pos + len) % TX_BUFFER_SIZE] =  data[characters];
			len++;
		}

		palExec_atomicBegin();

		txLength = len;

		if (txLength > TX_BUFFER_SIZE)
			txLength = TX_BUFFER_SIZE;

		palExec_atomicEnd();

		if (txLength > TX_BUFFER_SIZE / 2)
			flushTxBuffer();
		else
		{
			flush[Peripheral] = true;
			timer->start_async(2, timerFire);
		}

		return length;
	}

	/**
	 * Starts a dma transfer to send the buffer content
	 * If the dma controller is still active, the function starts
	 * a timer and is called again later
	 */
	void flushTxBuffer() {

		timer->stop();
		flush[Peripheral]  = false;

		palExec_atomicBegin();
		if (dma_active){
			timer->start_async(3, timerFire);
			flush[Peripheral] = true;
			palExec_atomicEnd();
			return;
		}

		dma_active = true;
		palExec_atomicEnd();

		if (dma_wrapAround){
			write_dma(txBuffer, dma_wrapLength, false);
			dma_wrapAround = false;
			return;
		}

		palExec_atomicBegin();
		uint8_t pos = txPos;
		uint8_t len = txLength;


		// in case the filled buffer wraps around,
		// a second transmission has to be started
		// after the first one
		if (pos + len > TX_BUFFER_SIZE){
			dma_wrapAround = true;
			len = TX_BUFFER_SIZE - pos;
			dma_wrapLength = txLength - len;
		}


		if (len > 0)
			write_dma(txBuffer + pos, len, false);

		txPos = (pos + txLength)  % TX_BUFFER_SIZE;
		txLength = 0;
		flush[Peripheral] = 0;

		palExec_atomicEnd();
	}

	/**
	 *	Starts the dma transmission
	 */
	inline bool write_dma(const uint8_t* data, uint8_t length, bool flagFirst = true)
	{

		configureDMA((uint32_t) data, length);

		//Clear DMA ItPendingBit
		switch((uint32_t)txDmaChannel)
		{
		case (uint32_t)DMA1_Channel4:
			NVIC_EnableIRQ(DMA1_Channel4_IRQn);
			DMA_ClearITPendingBit(DMA1_IT_TC4);
			break;
		case (uint32_t) DMA1_Channel2:
			NVIC_EnableIRQ(DMA1_Channel2_IRQn);
			DMA_ClearITPendingBit(DMA1_IT_TC2);
			break;
		case (uint32_t) DMA1_Channel7:
			DMA_ClearITPendingBit(DMA1_IT_TC7);
			NVIC_EnableIRQ(DMA1_Channel7_IRQn);
			break;
		case (uint32_t) DMA2_Channel5:
			DMA_ClearITPendingBit(DMA2_IT_TC5);
			NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);
			break;
		default:
			break;
		}

		//Enable DMA
		palExec_atomicBegin();

		DMA_ITConfig(txDmaChannel, DMA_IT_TC, ENABLE);
		DMA_Cmd(txDmaChannel, ENABLE);
		palExec_atomicEnd();
		return true;
	}

	/**
	 *	Copies data from the RX buffer into the given buffer.
	 *
	 *	@return The number of bytes written into the buffer
	 *	@param data The base address of the target buffer
	 *	@param length The maximum number of bytes that can
	 *			be written into the target buffer
	 */
	inline uint8_t read(uint8_t* data, uint8_t length)
	{
		uint8_t min = length;

		palExec_atomicBegin();
		if (rxLength < length)
			min = rxLength;
		palExec_atomicEnd();

		uint8_t i;

		int16_t buffer_pos = ((int16_t) rxPos) - min;

		if (buffer_pos < 0)
			buffer_pos += RX_BUFFER_SIZE;

		for (i = 0; i < min; i++)
		{
			data[i] = rxBuffer[buffer_pos];
			buffer_pos = (buffer_pos + 1) % RX_BUFFER_SIZE;
		}

		palExec_atomicBegin();
		rxLength -= min;
		palExec_atomicEnd();

		return min;
	}


	/**
	 * TxReady callback is called, if the txBuffer is empty
	 */
	void txReadyISR() {
		DISABLE_UART_TX_RDY_INT();
		if (txLength == 0) {
			if(txReadyCallback != NULL) {
				txReadyCallback->invoke();
			}
		}
	}

	/**
	 * Should not be called in DMA mode, since tx is handled by dma-interrupts
	 */
	void txFifoISR() {
		DISABLE_UART_TX_FIFO_INT();
	}

	/**
	 * Should be called, when a new byte has been received,
	 * copies the byte from data register into rxBuffer and
	 * increases the rxLength
	 */
	void rxISR() {

		palExec_atomicBegin();
		rxBuffer[rxPos] = (uint8_t) (usart->DR & 0xFF);
		rxPos = (rxPos + 1) % RX_BUFFER_SIZE;

		if (rxLength < RX_BUFFER_SIZE){
			rxLength++;
		}

		palExec_atomicEnd();

		if (rxLength == 1 && rxStartCallback != NULL) {
			ENABLE_UART_RX_INT();
			rxStartCallback->invoke();
		}
	}

	void isr(){

		if (usart->CR1 & USART_CR1_RXNEIE)
		{
			DISABLE_UART_RX_INT();
			if ((usart->SR & USART_SR_RXNE) || (usart->SR & USART_SR_ORE)) {
				rxISR();
				if (usart->SR & USART_SR_ORE)
					palLed_toggle(4);
			}
			ENABLE_UART_RX_INT();
		}

		if ((usart->CR1 & USART_CR1_TXEIE) && (usart->SR & USART_SR_TXE ))
			txFifoISR();

		if ((usart->CR1 & USART_CR1_TCIE) && (usart->SR & USART_SR_TC ))
			txReadyISR();

	}

	void dma_tc_isr () {

		dma_active = false;

		// if previous transmission left something in the
		// buffer
		if (dma_wrapAround)
		{
			flushTxBuffer();
			return;
		}

		if (txFifoEmptyCallback != 0){
			txFifoEmptyCallback->invoke();
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

//------------------- Interrupt handlers -------------------------------
//Interrupt handler declaration in C/C++
#ifdef __cplusplus
extern "C" {
#endif

void USART1_IRQHandler();
void USART2_IRQHandler();
void USART3_IRQHandler();
void USART4_IRQHandler();
void USART5_IRQHandler();
void DMA1_Channel4_IRQHandler();
void DMA1_Channel2_IRQHandler();
void DMA1_Channel7_IRQHandler();
void DMA2_Channel5_IRQHandler();

#ifdef __cplusplus
}
#endif



//-------------- INTERRUPT HANDLERS FOR DMA AND USART INTERRUPTS ------------------
void DMA1_Channel4_IRQHandler() {
	palExec_atomicBegin();
	if (DMA_GetITStatus(DMA1_IT_TC4))
	{
		DMA_ClearITPendingBit(DMA1_IT_TC4);
		PalSerialImpl<1>::getInstance().dma_tc_isr();
	}
	palExec_atomicEnd();
}


void DMA1_Channel2_IRQHandler() {
	if (DMA_GetITStatus(DMA1_IT_TC2))
	{
        DMA_ClearITPendingBit(DMA1_IT_TC2);
		PalSerialImpl<3>::getInstance().dma_tc_isr();
	}
}

void DMA1_Channel7_IRQHandler() {
	if (DMA_GetITStatus(DMA1_IT_TC7))
	{
        DMA_ClearITPendingBit(DMA1_IT_TC7);
		PalSerialImpl<2>::getInstance().dma_tc_isr();
	}
}

void DMA2_Channel5_IRQHandler() {
	if (DMA_GetITStatus(DMA2_IT_TC5))
	{
        DMA_ClearITPendingBit(DMA2_IT_TC5);
		PalSerialImpl<4>::getInstance().dma_tc_isr();
	}
}

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

/**
 * Timer for flushing the buffer.
 */
void timerFire(){
	if (flush[1])
			cometos::PalSerialImpl<1>::getInstance().flushTxBuffer();
	if (flush[2])
			cometos::PalSerialImpl<2>::getInstance().flushTxBuffer();
	if (flush[3])
			cometos::PalSerialImpl<3>::getInstance().flushTxBuffer();
	if (flush[4])
			cometos::PalSerialImpl<4>::getInstance().flushTxBuffer();
	if (flush[5])
			cometos::PalSerialImpl<5>::getInstance().flushTxBuffer();
}



