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
 * Based on Texas Instruments CC11xL Api
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * CometOS Platform Abstraction Layer for SPI communication and interrupts
 * with the CC110L longrange chip.
 *
 * @author Emil Kallias
 */


#include "CCSpi.h"
#include "cc_common.h"
#include "palSpi.h"

namespace cometos {

CCSpi::CCSpi(IGPIOPin* chipSelect, IGPIOPin* miso)
: chipSelect(chipSelect),
miso(miso) {
}

/******************************************************************************
 * @fn          CCSpi::interfaceInit
 *
 * @brief       Function to initialize TRX SPI. CC1101/CC112x is currently
 *              supported. The supported clockDivider must be set so that
 *              SMCLK/clockDivider does not violate radio SPI constraints.
 *
 * input parameters
 *
 * @param       clockDivider - SMCLK/clockDivider gives SCLK frequency
 *
 * output parameters
 *
 * @return      void
 */

#define CCSPI_SPI_FREQUENCY 1000000

void CCSpi::interfaceInit(void) {

    /* Set MOSI, SCK and SS output, all others input */
    //DDR(SPI_SS_PORT) = (1<<SPI_SS_PIN);
    //SPI_SS_PORT |= (1<<SPI_SS_PIN); // pull down

    chipSelect->set();
    chipSelect->setDirection(IGPIOPin::Direction::OUT);


    //DDR(SPI_PORT) = (1<<SPI_MOSI_PIN)|(1<<SPI_SCK_PIN);
    //SPI_PORT |= (1<<SPI_MOSI_PIN); // pull down

    /* Enable SPI, Master, set clock rate fck/16 */
    //SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}



/*******************************************************************************
 * @fn          CCSpi::regAccess
 *
 * @brief       This function performs a read or write from/to a 8bit register
 *              address space. The function handles burst and single read/write
 *              as specfied in addrByte. Function assumes that chip is ready.
 *
 * input parameters
 *
 * @param       accessType - Specifies if this is a read or write and if it's
 *                           a single or burst access. Bitmask made up of
 *                           BURST_ACCESS/SINGLE_ACCESS/
 *                           WRITE_ACCESS/READ_ACCESS.
 * @param       addrByte - address byte of register.
 * @param       pData    - data array
 * @param       len      - Length of array to be read(TX)/written(RX)
 *
 * output parameters
 *
 * @return      chip status
 */
ccStatus_t CCSpi::readAccess(uint8_t addrByte, uint8_t *pData, uint16_t len, bool burst)
{
    if(len <= 0) {
	// if len is 0, the buffer sizes are also 0, so the transmitBlocking will write to an unsafe memory location
	return 0;
    }

    uint8_t accessType = READ_ACCESS;

    if(burst) {
        accessType |= BURST_ACCESS;
    }

    uint8_t header;
    ccStatus_t readValue;

    cometos_error_t ret = palSpi_init(true, CCSPI_SPI_FREQUENCY, 0, false);
    ASSERT(ret == COMETOS_SUCCESS);
    miso->pullup(true);

    chipSelect->clear();

    // transmit header
    header = accessType|addrByte;
    palSpi_transmitBlocking(&header, &readValue, 1);

    // read via SPI
    palSpi_transmitBlocking(NULL, pData, len);

    chipSelect->set();

    return readValue;
}

ccStatus_t CCSpi::writeAccess(uint8_t addrByte, const uint8_t *pData, uint16_t len, bool burst)
{
    if(len <= 0) {
	// if len is 0, the buffer sizes are also 0, so the transmitBlocking will write to an unsafe memory location
	return 0;
    }

    uint8_t accessType = WRITE_ACCESS;

    if(burst) {
        accessType |= BURST_ACCESS;
    }

    uint8_t header;
    ccStatus_t readValue;

    cometos_error_t ret = palSpi_init(true, CCSPI_SPI_FREQUENCY, 0, false);
    ASSERT(ret == COMETOS_SUCCESS);
    miso->pullup(true);

    chipSelect->clear();

    // transmit header
    header = accessType|addrByte;
    palSpi_transmitBlocking(&header, &readValue, 1);

    // read via SPI
    palSpi_transmitBlocking(pData, NULL, len);

    chipSelect->set();

    return readValue;
}

/*******************************************************************************
 * @fn          CCSpi::cmdStrobe
 *
 * @brief       Send command strobe to the radio. Returns status byte read
 *              during transfer of command strobe. Validation of provided
 *              is not done. Function assumes chip is ready.
 *
 * input parameters
 *
 * @param       cmd - command strobe
 *
 * output parameters
 *
 * @return      status byte
 */
ccStatus_t CCSpi::cmdStrobe(uint8_t cmd)
{
    uint8_t rxBuf;

    cometos_error_t ret = palSpi_init(true, CCSPI_SPI_FREQUENCY, 0, false);
    ASSERT(ret == COMETOS_SUCCESS);
    miso->pullup(true);

    //CC_SPI_BEGIN();
    chipSelect->clear();

    palSpi_transmitBlocking(&cmd, &rxBuf, 1);

    chipSelect->set();
    //CC_SPI_END();
    return(rxBuf);
}


} /* namespace cometos */

