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

#ifndef N25Q128A_H
#define N25Q128A_H

#include "cometosError.h"
#include "cometosAssert.h"
#include "IGPIOPin.h"
#include "palSpi.h"
#include "n25q128a_commands.h"

//------- MACRO DEFINITIONS ------------------------
#define N25Q128A_PAGE_SIZE 256
#define N25Q128A_SECTOR_SIZE 65536
#define N25Q128A_SIZE 65536 * 256

#define N25Q128A_STATUS_WRITE_IN_PROGRESS 0x1

#define NUM_SWAP_SECTORS 25
#define NUM_SECTORS 256

//------- CLASS DEFINITION ------------------------


namespace cometos {

class N25xx {

public:

	/**
	 * Initializes the Flash and the used pin.
	 * SPI is expected to already be initialized with correct parameters.
	 * Pins will be configured as output pins and initialized as set.
	 *
	 * @param spi An implementation of the PalSpi class to handle communication with the Flash.
	 * @param holdn_pin The IGPIOPin Object to control the pin connected to the holdn input of the Flash.
	 * @param wn_pin The IGPIOPin Object connected to the wn pin of the Flash.
	 */
	cometos_error_t init(PalSpi* spi, IGPIOPin* holdn_pin, IGPIOPin* wn_pin)
	{
		this->spi = spi;
		this->holdn_pin = holdn_pin;
		this->wn_pin = wn_pin;

		holdn_pin->setDirection(IGPIOPin::Direction::OUT);
		wn_pin->setDirection(IGPIOPin::Direction::OUT);

		wn_pin->set();
		holdn_pin->set();

		return COMETOS_SUCCESS;
	}

	/**
	 * Reads the devices identification data into a buffer.
	 * The identification data contains details as Manufacturer,
	 * memory type and capacity. See datasheet for more details.
	 *
	 * @param buffer The target buffers start address in the MCUs memory
	 * @param The number of bytes to transfer.
	 */
	void readId(uint8_t* buffer, uint16_t len ){
		uint8_t instruction = N25Q128A_CMD_RDID;

		spi->enable();
		spi->transmitBlocking(&instruction, buffer, 1);

		uint8_t i;
		for(i = 0; i < len; i++)
			spi->transmitBlocking(&instruction, buffer + i, 1);

		spi->disable();
	}

	/**
	 * Transfers data from the Flash memory into a buffer in the MCUs memory.
	 *
	 * @param address Target Address in the Flash memory to start reading from.
	 * @param buf Start Address of the destination buffer in the MCUs memory.
	 * @param len Number of bytes to transfer.
	 */
	cometos_error_t read(uint32_t address, uint8_t* buf, uint16_t len){
		uint8_t instructionBuffer[4]; //three bytes, since 128MBit flash address space fits in only 3 bytes
		instructionBuffer[0] = N25Q128A_CMD_READ;
		instructionBuffer[1] = address >> 16;
		instructionBuffer[2] = address >> 8;
		instructionBuffer[3] = address;

		spi->enable();

		spi->transmitBlocking(instructionBuffer, buf, 4);

		uint16_t i;
		for (i = 0; i < len; i++){
			spi->transmitBlocking(instructionBuffer, buf + i, 1);
		}

		spi->disable();

		return COMETOS_SUCCESS;
	}

	/**
	 *	@return The Status register of the Flash.
	 */
	uint8_t readStatus() {

		uint8_t transmitBuffer[2] = { N25Q128A_CMD_RDSR, 0};
		uint8_t receiveBuffer[2];

		spi->enable();
		spi->transmitBlocking(transmitBuffer, receiveBuffer, 2);
		spi->disable();

		return receiveBuffer[1];
	}

	/**
	 * Returns an instance of this class. The singleton
	 * pattern is enforced.
	 */
	static N25xx* getInstance(){
		static N25xx singleton;

		return &singleton;
	}

	/**
	 * This sets the write enable latch on the Flash, which
	 * must be done before any programming or erasing action.
	 */
	void writeEnable() {
		uint8_t txBuffer, rxBuffer;
		txBuffer = N25Q128A_CMD_WREN;

		spi->enable();
		spi->transmitBlocking(&txBuffer, &rxBuffer, 1);
		spi->disable();
	}

	/**
	 * Disables the write-enable latch on the flash.
	 */
	void writeDisable() {
		uint8_t txBuffer, rxBuffer;
		txBuffer = N25Q128A_CMD_WRDI;

		spi->enable();
		spi->transmitBlocking(&txBuffer, &rxBuffer, 1);
		spi->disable();
	}

	/**
	 * Programs one complete page. Programming can only flip bits from 1 to 0.
	 * If the contains data that need to be overwritten, an erase has to be done
	 * first manually, or use the write function instead, which already
	 * includes erasing.
	 * The functions blocks, until the flash signals that the write has been
	 * completed.
	 *
	 * @param address Start address in the flash memory to write to.
	 * @param buf Start address of the source buffer in the MCUs memory.
	 * @param length Number of bytes to write.
	 */
	void programPage(uint32_t address, uint8_t *buf, uint16_t length) {
		uint8_t instructionBuffer[4]; //three bytes, since 128MBit flash address space fits in only 3 bytes
		instructionBuffer[0] = N25Q128A_CMD_PP;
		instructionBuffer[1] = address >> 16;
		instructionBuffer[2] = address >> 8;
		instructionBuffer[3] = address;

		uint8_t receiveBuff[4];

		writeEnable();

		spi->enable();
		spi->transmitBlocking(instructionBuffer, receiveBuff, 4);

		uint16_t i;
		for (i = 0; i < length; i++){
			spi->transmitBlocking(buf + i, receiveBuff, 1);
		}

		spi->disable();

		//check for finish signal:

		while (readStatus() & N25Q128A_STATUS_WRITE_IN_PROGRESS)
			__asm("nop");
	}

	/**
	 *	Writes data to flash memory. This includes erasing the pages before
	 *	programming.
	 *
	 *	@param address Start address to write to in the flash memory.
	 *	@param buffer Start address of the source buffer in the MCUs memory.
	 *	@param length Number of bytes to store.
	 */
	cometos_error_t write(uint32_t address, uint8_t *buffer, uint16_t length){
		 if (address + length >= N25Q128A_SIZE) {
		        return COMETOS_ERROR_INVALID;
		    }

		    uint32_t firstPage = address / N25Q128A_PAGE_SIZE;
		    uint32_t finalAddr = address + length - 1;
		    uint32_t finalPage = finalAddr / N25Q128A_PAGE_SIZE;

		    for(uint32_t page = firstPage; page <= finalPage; page++) {

		        // calculate address and length
		        uint32_t currAddr = page*N25Q128A_PAGE_SIZE;
		        uint16_t currLen = N25Q128A_PAGE_SIZE;

		        if(page == firstPage) {
		            // first page
		            currLen = N25Q128A_PAGE_SIZE - (address - currAddr);
		            currAddr = address;
		        }

		        if(page == finalPage) { // can also be at the same time as the first page
		            currLen = finalAddr - currAddr + 1;
		        }

		        // do the write
		        programPage(currAddr, buffer, currLen);

		        // increase the buffer pointer
		        buffer += currLen;
		    }

		    return COMETOS_SUCCESS;
	}

	/**
	 * Copies all pages from one sector to another one.
	 * The destination sector is getting erased during this function.
	 *
	 * @param src Any address lying within the target sector.
	 * @param dest Any address lying within the destination sector.
	 */
	void copySector(uint32_t src, uint32_t dest) {
		//copy page by page
		uint8_t pageBuffer[N25Q128A_PAGE_SIZE];

		writeEnable();
		eraseSector(dest);

		uint16_t i;
		for (i = 0; i < N25Q128A_SECTOR_SIZE / N25Q128A_PAGE_SIZE; i++){
			read(src , pageBuffer, N25Q128A_PAGE_SIZE );

			writeEnable();
			programPage(dest, pageBuffer, N25Q128A_PAGE_SIZE);

			src += N25Q128A_PAGE_SIZE;
			dest += N25Q128A_PAGE_SIZE;
		}
	}

	/**
	 * Erases a sector. This function blocks until erasure has been
	 * completed.
	 *
	 * @param address Any address is valid and erases the sector it belongs to
	 */
	cometos_error_t eraseSector(uint32_t address) {

		uint8_t instructionBuffer[4]; //three bytes, since 128MBit flash address space fits in only 3 bytes
		instructionBuffer[0] = N25Q128A_CMD_SE;
		instructionBuffer[1] = address >> 16;
		instructionBuffer[2] = address >> 8;
		instructionBuffer[3] = address;

		uint8_t receiveBuff[4];

		writeEnable();

		spi->enable();
		spi->transmitBlocking(instructionBuffer, receiveBuff, 4);
		spi->disable();

		while (readStatus() & N25Q128A_STATUS_WRITE_IN_PROGRESS)
			__asm("nop");

		return COMETOS_SUCCESS;
	}

	/**
	 * Erases a single subsector.
	 * Only available in Top or Bottom versions of the chip. See
	 * Datasheet for more information.
	 */
	void eraseSubsector(uint32_t address) {
		uint8_t instructionBuffer[4]; //three bytes, since 128MBit flash address space fits in only 3 bytes
		instructionBuffer[0] = N25Q128A_CMD_SSE;
		instructionBuffer[1] = address >> 16;
		instructionBuffer[2] = address >> 8;
		instructionBuffer[3] = address;

		uint8_t receiveBuff[4];

		writeEnable();

		spi->enable();
		spi->transmitBlocking(instructionBuffer, receiveBuff, 4);
		spi->disable();

		while (readStatus() & N25Q128A_STATUS_WRITE_IN_PROGRESS)
			__asm("nop");
	}

	/**
	 * Erases the full flash.
	 */
	void bulkErase(){
		uint8_t txBuffer, rxBuffer;
		txBuffer = N25Q128A_CMD_BE;

		writeEnable();

		spi->enable();
		spi->transmitBlocking(&txBuffer, &rxBuffer, 1);
		spi->disable();

		while (readStatus() & N25Q128A_STATUS_WRITE_IN_PROGRESS)
					__asm("nop");
	}

private:

	N25xx(){};
	~N25xx(){};

	PalSpi* spi;
	IGPIOPin* holdn_pin;
	IGPIOPin* wn_pin;

};

}
#endif
