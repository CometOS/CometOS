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

#include "palSpi.h"
#include "cmsis_device.h"
#include "cometosAssert.h"

namespace cometos {


/**
 * Specialization of the PalSpi class for the STM32 microcontroller
 *
 * TODO: Implement asynchronous communication
 * TODO: Implement slave mode (receive callback?)
 */
template<int Peripheral>
class PalSpiImplementation : public PalSpi {

private:

	SPI_TypeDef* spi;

	IGPIOPin* csPin;
	bool csActiveLow;
	bool periphClock1;

	volatile bool enabled;

	uint16_t txBufferPos;
	uint16_t rxBufferPos;
	uint16_t txBufferLength;
	uint16_t rxBufferLength;


public:
	cometos_error_t init(bool asMaster, uint32_t freq, palSpi_mode_t mode, bool lsbFirst){

		uint32_t periphClk;

		if (Peripheral >= 2){
            uint8_t clockPeripheral = Peripheral - 2;
			RCC_APB1PeriphClockCmd( (RCC_APB1Periph_SPI2 << clockPeripheral), ENABLE);
			periphClk = SystemCoreClock / 2;
		}
		else {
			RCC_APB2PeriphClockCmd( RCC_APB2Periph_SPI1, ENABLE);
			periphClk = SystemCoreClock;
		}

		switch(Peripheral)
		{
		case 1:
			spi = SPI1;
			break;
		case 2:
			spi = SPI2;
			break;
		case 3:
			spi = SPI3;
			break;
		default:
			return COMETOS_ERROR_FAIL;
			break;
		}

		//init typedef to fill with init information
		SPI_InitTypeDef spiInit;

		spiInit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;

		if (asMaster)
		{
			spiInit.SPI_Mode = SPI_Mode_Master;

			//Calculate and set prescaler for peripheral
			uint32_t exact_prescaler = periphClk / freq;

			if (exact_prescaler >= 256)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
			else if (exact_prescaler >= 128)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
			else if (exact_prescaler >= 64)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
			else if (exact_prescaler >= 32)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
			else if (exact_prescaler >= 16)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
			else if (exact_prescaler >= 8)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
			else if (exact_prescaler >= 4)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
			else if (exact_prescaler >= 2)
				spiInit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
			else
				return COMETOS_ERROR_INVALID;

			//Set ChipSelect signal controlled by software
			spiInit.SPI_NSS = SPI_NSS_Soft;
		}
		else
		{
			spiInit.SPI_Mode = SPI_Mode_Slave;
		}

		if (lsbFirst)
			spiInit.SPI_FirstBit = SPI_FirstBit_LSB;
		else
			spiInit.SPI_FirstBit = SPI_FirstBit_MSB;

		// Set Clock polarity mode:
		switch (mode)
		{
		case 0:
			spiInit.SPI_CPOL = SPI_CPOL_Low;
			spiInit.SPI_CPHA = SPI_CPHA_1Edge;
			break;
		case 1:
			spiInit.SPI_CPOL = SPI_CPOL_Low;
			spiInit.SPI_CPHA = SPI_CPHA_2Edge;
			break;
		case 2:
			spiInit.SPI_CPOL = SPI_CPOL_High;
			spiInit.SPI_CPHA = SPI_CPHA_1Edge;
			break;
		case 3:
			spiInit.SPI_CPOL = SPI_CPOL_High;
			spiInit.SPI_CPHA = SPI_CPHA_2Edge;
			break;
		default:
			break;
		}

		spiInit.SPI_DataSize = SPI_DataSize_8b;

		spiInit.SPI_CRCPolynomial = 7;

		SPI_Init(spi, &spiInit);

		enabled = false;

		return COMETOS_SUCCESS;
	}

	void enable()
	{
		palExec_atomicBegin();
		ASSERT(!enabled);
		enabled = true;

		SPI_Cmd(spi, ENABLE);

		if (csActiveLow){
			csPin->clear();
		}
		else{
			csPin->set();
		}
		palExec_atomicEnd();
	}

	void disable()
	{
		palExec_atomicBegin();
		SPI_Cmd(spi, DISABLE);

		if (csActiveLow){
			csPin->set();
		}
		else{
			csPin->clear();
		}

		enabled = false;

		//Delay to ensure blocking time of signal
		for (uint8_t i = 0; i < 20; i++)
			__asm("nop");

		palExec_atomicEnd();
	}

	void setCsPin(IGPIOPin* pin, bool active_low){
		this->csActiveLow = active_low;
		this->csPin = pin;

		csPin->setDirection(IGPIOPin::Direction::OUT);

		if (csActiveLow){
			csPin->set();
		}
		else{
			csPin->clear();
		}
	}

	cometos_error_t swapByte(uint8_t tx, uint8_t *rx){
		return transmitBlocking(&tx, rx, 1);
	}

	/**
	 * NOT IMPLEMENTED YET
	 */
	cometos_error_t transmit(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len, palSpi_cbPtr callback){
		return COMETOS_ERROR_FAIL;
	};

	cometos_error_t transmitBlocking(const uint8_t *txBuf, uint8_t* rxBuf, uint16_t len){
		uint16_t txPos = 0;
		uint16_t rxPos = 0;

		while (txPos < len){
			//Write to the SPI Data register as soon as free
			while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) != SET ){
				__asm("nop");
			}
			spi->DR = txBuf[txPos++];

			//Read from the SPI Data register as soon as data is available
            uint16_t i = 0;
			while (! (spi->SR & SPI_I2S_FLAG_RXNE)) {//SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE != SET)){
				__asm("nop");
                ASSERT(i++ < 1000);
			}

			rxBuf[rxPos++] = spi->DR;
		}
		return COMETOS_SUCCESS;
	}

	bool isAvailable() {
		return !enabled;
	}

	static PalSpiImplementation& getInstance() {
		static PalSpiImplementation<Peripheral> spi;
		return spi;
	}

protected:
	PalSpiImplementation() {
		spi = NULL;
	}
};

template<> PalSpi* PalSpi::getInstance<int>(int peripheral) {
	switch (peripheral){
	case 1:
		return &PalSpiImplementation<1>::getInstance();
	case 2:
		return &PalSpiImplementation<2>::getInstance();
	case 3:
		return &PalSpiImplementation<3>::getInstance();
	default:
		return NULL;
	}
}

}
