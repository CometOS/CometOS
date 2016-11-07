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

#include "CC110lApi.h"
#include "palExec.h"

#include <string.h>

namespace cometos {

/* Minimum definitions for reading chip ID and chip VERSION */
#define CC1101_READ_BURST               0xC0
#define CC1101_PARTNUM_ADDR             0x30
#define CC1101_VERSION_ADDR             0x31

/* Bit fields in the chip status byte */
#define STATUS_CHIP_RDYn_BM             0x80
#define STATUS_STATE_BM                 0x70
#define STATUS_FIFO_BYTES_AVAILABLE_BM  0x0F

CC110lApi::CC110lApi(IGPIOPin* gdo0pin, IGPIOPin* chipSelectPin, IGPIOPin* misoPin, Callback<void()> setupRoutine) :
    CCSpi(chipSelectPin,misoPin),
    readTask(*this),
    readoutFIFOTask(*this),
    packetPending(false),
    packetMode(true),
    setupRoutine(setupRoutine),
    gdo0pin(gdo0pin),
    channel(0),
    hasFixedLength(false),
    fixedLength(0),
    previousState(CC110L_STATE_IDLE)
{
    if(setupRoutine == EMPTY_CALLBACK()) {
        setupRoutine = CALLBACK_MET(&CC110lApi::setup,*this);
    }

    packetStartCallback = EMPTY_CALLBACK();
    packetEndCallback = EMPTY_CALLBACK();
    dataAvailableCallback = EMPTY_CALLBACK();

    gdo0pin->setDirection(IGPIOPin::Direction::IN);
    gdo0pin->pullup(true);
}

void CC110lApi::enableAmp()
{
    // Enable these options if cc1190 is available and should amplify the signal
    // For AutoRNode these shall not be enabled and interfer with the RS485!
    cc1190.init();
    cc1190.enable();
    cc1190.highGainEnable();
}

void CC110lApi::setup()
{
    interfaceInit();

    cmdStrobe(CC110L_SRES);
    regConfig();
    setUnamplifiedOutputPower(getMaxUnamplifiedOutputPower());
    setFrequency(869430);
    setDataRate(1000);
    setCRC(false);
    setCCAMode(CC110lApi::CCAMode::ALWAYS);
    setChannelSpacing(60000);
    setChannelBandwidth(60268); // minimum channel width for 27 MHz
    setDeviation(10000);

    setSync(CC110lApi::Sync::SYNC_30_32);
    setPreamble(24);
    setCoding(CC110lApi::Coding::UNCODED);

    setFrequencyOffset(0); // is shifted in setFrequencyOffset by board specific value

    stateChangeConnect(CALLBACK_MET(&CC110lApi::callStateChangeCallback,*this));

    /* Function is called if data has been received successfully */
    setDataAvailableCallback(CALLBACK_MET(&CC110lApi::interpreteRxData,*this)); /* must be a function not a method */
    cmdStrobe(CC11xL_SNOP);
    enterSleep();

    printChipType();

    setupRoutine();
}

void CC110lApi::receivePacket(receive_callback_t receiveCallback) {
    this->receiveCallback = receiveCallback;
    this->receiveCallbackExt = EMPTY_CALLBACK();
    CC110lApi::receivePacket();
}

void CC110lApi::receivePacket(receive_callback_ext_t receiveCallback) {
    this->receiveCallback = EMPTY_CALLBACK();
    this->receiveCallbackExt = receiveCallback;
    receivePacket();
}

cometos_error_t CC110lApi::send(uint8_t length, const uint8_t *pData, Callback<void()> txReadyCallback)
{
    cometos_error_t result = preparePacket(length, pData, txReadyCallback);

    if(result != COMETOS_SUCCESS) {
        return result;
    }

    return send();
}

void CC110lApi::printChipType(void)
{
    CC110lApi::radioChipType_t radioType;
    detectChipType(&radioType);
}

void CC110lApi::interpreteRxData(uint8_t length, uint8_t *pData, int16_t rssi, uint8_t crc_ok)
{
    enterSleep();
    if(receiveCallback) {
        receiveCallback(length, pData);
    }
    if(receiveCallbackExt) {
        receiveCallbackExt(length, pData, rssi, crc_ok);
    }
}

void CC110lApi::callStateChangeCallback(ccStatus_t newState) {
    if(newState != previousState && previousState == CC110L_STATE_TX) {
        if(txReadyCallback) {
            txReadyCallback();
            txReadyCallback = EMPTY_CALLBACK();
        }
    }

    switch(newState) {
        case CC110L_STATE_TX:
            if((cc1190.cc1190Status != CC1190_NA) && (cc1190.cc1190Status != CC1190_DISABLED)) {
                cc1190.enterTX();
            }
            break;
        case CC110L_STATE_RX:
            if((cc1190.cc1190Status != CC1190_NA) && (cc1190.cc1190Status != CC1190_DISABLED)) {
                cc1190.enterRX();
            }
            break;
            // TODO in Idle disable amp
    }

    previousState = newState;
}

uint8_t CC110lApi::detectChipType(radioChipType_t *pRadioChipType)
{
    uint8_t id;
    uint8_t ver;
    uint16_t type;

    readAccess(CC1101_PARTNUM_ADDR, &id, 1);
    readAccess(CC1101_VERSION_ADDR, &ver, 1);

    id++;
    if(id == 0x00 )
    {
        switch(ver)
        {
        case 0x04:
            type = CHIP_TYPE_CC1101;
            break;
        case 0x07:
            type = CHIP_TYPE_CC110L;
            break;
        case 0x08:
            type = CHIP_TYPE_CC113L;
            break;
        case 0x09:
            type = CHIP_TYPE_CC115L;
            break;
        default:
            type = CHIP_TYPE_NONE;
        }
    }
    else if(id == 0x80 )
    {
        switch(ver)
        {
        case 0x03:
            type = CHIP_TYPE_CC2500;
            break;
        default:
            type = CHIP_TYPE_NONE;
        }
    }
    else
    {
        type = CHIP_TYPE_NONE;
    }

    // Populating the global radio device struct if specific radio was detected
    if(type != CHIP_TYPE_NONE)
    {
        pRadioChipType->id = id;
        pRadioChipType->ver = ver;
        pRadioChipType->deviceName = type;
    }
    else
    {
        // Defaulting the struct values if a radio chip is not detected
        pRadioChipType->id  = 0x00;
        pRadioChipType->ver = 0x00;
        pRadioChipType->deviceName =  CHIP_TYPE_NONE;
        return 0;
    }

    return 1;
}

cometos_error_t CC110lApi::preparePacket(uint8_t len, const uint8_t *pData, Callback<void()> txReadyCallback)
{
    this->txReadyCallback = txReadyCallback;

    if(hasFixedLength && len != fixedLength) {
        // wrong length
        return COMETOS_ERROR_FAIL;
    }

    if(getState() == CC110L_STATE_TX) {
        uint8_t status = cmdStrobe(CC110L_SNOP);
        if(status & STATUS_CHIP_RDYn_BM) {
            // chip not ready
            return COMETOS_ERROR_FAIL;
        }
        else if((status & STATUS_STATE_BM) == CC110L_STATE_TX) {
            // chip busy
            return COMETOS_ERROR_FAIL;
        }
        else if((status & STATUS_FIFO_BYTES_AVAILABLE_BM) == 0) {
            // FIFO full
            return COMETOS_ERROR_FAIL;
        }
        else {
            // state is invalid -> set to correct state
            rfState = CC110L_STATE_IDLE;
            setupRoutine();
            return COMETOS_ERROR_FAIL;
        }
    }


    /* Will only try to transmit if the whole packet can fit i RXFIFO
     * and we're not currently sending a packet.
     */
    if(!(len > (PER_MAX_DATA-2)) && (getState() != CC110L_STATE_TX) )
    {
        if(!hasFixedLength) {
            writeAccess(CC11xL_FIFO, &len, 1);
        }
        writeAccess(CC11xL_FIFO, pData, len);

        uint8_t data;
        readAccess(CC110L_PKTCTRL0,&data,1);
        data &= ~(0x30);
        data |= (0x00);
        writeAccess(CC110L_PKTCTRL0,&data,1);

        packetPending = true;
        return COMETOS_SUCCESS;
    }
    else {
        // busy
        return COMETOS_ERROR_FAIL;
    }
}

void CC110lApi::sendInfinite() {
    uint8_t data;

	// Infinite packet length mode and CRC off
	readAccess(CC110L_PKTCTRL0,&data,1);
	data = (data & ~0x7) | 0x2;
	writeAccess(CC110L_PKTCTRL0,&data,1);

    changeState(CC110L_STATE_TX);
    cmdStrobe(CC110L_STX);
}

void CC110lApi::fillFIFO(uint8_t* data, uint8_t len) {
    writeAccess(CC11xL_FIFO, data, len);
}

cometos_error_t CC110lApi::send()
{
    if(packetPending) {
        packetPending = false;
        changeState(CC110L_STATE_TX);
        /* Indicate state to the ISR and issue the TX strobe */

        // setup for falling edge
        cometos_error_t ret = gdo0pin->setupInterrupt(IGPIOPin::Edge::FALLING, CALLBACK_MET(&CC110lApi::pinInterrupt,*this));
        ASSERT(ret == COMETOS_SUCCESS);

        cmdStrobe(CC110L_STX);
    }
    else {
        // no packet pending
        return COMETOS_ERROR_FAIL;
    }

    return COMETOS_SUCCESS;
}

void CC110lApi::changeState(ccStatus_t newState) {
    if(stateChangeCallback) {
        stateChangeCallback(newState);
    }
    rfState = newState;
}

ccStatus_t CC110lApi::getState() {
    return rfState;
}

uint8_t CC110lApi::getMode() {
    uint8_t data;
    readAccess(CC110L_MARCSTATE,&data,1);
    return data;
}

void CC110lApi::sendStatic() {
    uint8_t data;

    readAccess(CC110L_PKTCTRL0,&data,1);
    data &= ~(0x30);
    data |= (0x30);
    writeAccess(CC110L_PKTCTRL0,&data,1);

    changeState(CC110L_STATE_TX);
    cmdStrobe(CC110L_STX);
}

void CC110lApi::receivePacket(void)
{
	packetMode = 1;

    // setup for falling edge
    cometos_error_t ret = gdo0pin->setupInterrupt(IGPIOPin::Edge::ANY_EDGE, CALLBACK_MET(&CC110lApi::pinInterrupt,*this));
    ASSERT(ret == COMETOS_SUCCESS);
	
    enterIdle();
    //enableInt();
    enterRx();
}

void CC110lApi::receiveContinuously(void)
{
	packetMode = 0;

	enterIdle();
	enableInt();

	// Enable continuous mode
	uint8_t data;
	// Disable Sync
	readAccess(CC110L_MDMCFG2,&data,1);
	data = (data & ~0x7) | 0x0;
	writeAccess(CC110L_MDMCFG2,&data,1);

	// Infinite packet length mode and CRC off
	readAccess(CC110L_PKTCTRL0,&data,1);
	data = (data & ~0x7) | 0x2;
	writeAccess(CC110L_PKTCTRL0,&data,1);

	// Set GDO0 to RX FIFO threshold
	data = 0x0;
	writeAccess(CC110L_IOCFG0,&data,1);

	enterRx();

        // setup for rising edge
        cometos_error_t ret = gdo0pin->setupInterrupt(IGPIOPin::Edge::RISING, CALLBACK_MET(&CC110lApi::pinInterrupt,*this));
        ASSERT(ret == COMETOS_SUCCESS);
}

void CC110lApi::setDataAvailableCallback(dataAvailableCallback_t callback)
{
    dataAvailableCallback = callback;
}

void CC110lApi::setPacketStartCallback(packetStartCallback_t startCallback)
{
    packetStartCallback = startCallback;
}

void CC110lApi::setPacketEndCallback(packetEndCallback_t endCallback)
{
    packetEndCallback = endCallback;
}

int16_t CC110lApi::getRSSI(bool raw) {
    uint8_t RSSI;
    readAccess(CC110L_RSSI,(uint8_t*)&RSSI,1);
    if(raw) {
        return RSSI;
    }
    else {
        return convertRSSI(RSSI);
    }
}

int16_t CC110lApi::convertRSSI(uint8_t registerValue) {
    int16_t dezi_rssi = ((int16_t)((int8_t)registerValue))*(10/2)-cc11xLRssiOffset*10;
    return dezi_rssi;
}

uint8_t CC110lApi::getBytesInTxFIFO() {
    uint8_t bytes;
    readAccess(CC110L_TXBYTES,(uint8_t*)&bytes,1);
    bytes &= ~(1<<7); // ignore underflow bit
    return bytes;
}

void CC110lApi::readPacket()
{
    uint8_t fifo_length;
    uint8_t pData[255];

    // TODO error handling
    if(getState() == CC110L_STATE_RX) {
	    return;
    }

    // read offset compensation
    int8_t FREQEST;
    uint8_t MDMCFG4;
    readAccess(CC110L_FREQEST,(uint8_t*)&FREQEST,1);
    lastFREQEST = FREQEST;
    readAccess(CC110L_MDMCFG4,(uint8_t*)&MDMCFG4,1);
    //getCout() << "FREQEST " << dec << (int16_t)FREQEST << " MDMCFG4 0x" << hex << (uint16_t)MDMCFG4 << endl;

    readAccess(CC110L_RXBYTES,&fifo_length,1);

    cmdStrobe(CC11xL_SNOP | READ_ACCESS);
    readAccess(CC11xL_FIFO, pData, fifo_length);
    disableInt();

    //cometos::getCout() << "Received: " << dec << (uint16_t)fifo_length << " Bytes; " << (const char*) pData+1 << endl;

    // Only valid if APPEND_STATUS bit set
    int16_t dezi_rssi = convertRSSI(pData[fifo_length-2]);
    lastRSSI = dezi_rssi;
    
    uint8_t crc_ok = pData[fifo_length-1];

    enterSleep();

    if(dataAvailableCallback) {
	uint8_t* data_ptr = pData;
	if(!hasFixedLength) {
		// length byte in the first byte
		fifo_length -= 1;
		data_ptr += 1;
        }

        // rssi and crc_ok in the last two bytes
        fifo_length -= 2;

        dataAvailableCallback(fifo_length, data_ptr, dezi_rssi, crc_ok);
    }
}

void CC110lApi::enterRx(void)
{
    idleRx();

    changeState(CC110L_STATE_RX);
    return;
}

void CC110lApi::enterIdle(void)
{
    /* wait until chip is ready */
    //CC_SPI_BEGIN();
    //while(SPI_PORT & (1<<SPI_MISO_PIN));
    for(uint16_t i = 0; i < 1000; i++) {
    	if(!(cmdStrobe(CC110L_SNOP) & STATUS_CHIP_RDYn_BM)) {
	    break;
	}
    }

    if(cmdStrobe(CC110L_SNOP) & STATUS_CHIP_RDYn_BM) {
	// TODO error handling
	return;
    }

    rxIdle();
    changeState(CC110L_STATE_IDLE);
    return;
}

void CC110lApi::enterSleep(void)
{
    rxIdle();
    //TODO cc1190 dower down
    cmdStrobe(CC110L_SPWD);

    //getCout() << "CC110l sleep" << endl;

    /* Only important to differ between RX/TX and IDLE */
    changeState(CC110L_STATE_IDLE);
    return;
}

void CC110lApi::readoutFIFOWhileContinous() {
    uint8_t fifo_length;
    uint8_t pData[255];
    readAccess(CC110L_RXBYTES,&fifo_length,1);

    cmdStrobe(CC11xL_SNOP | READ_ACCESS);
    readAccess(CC11xL_FIFO, pData, fifo_length);

	if(fifo_length > 0) {
	    dataAvailableCallback(fifo_length, pData, 0xFF, 0xFF);
	}
}

void CC110lApi::pinInterrupt(void){
    if(packetMode) {
        bool rising = gdo0pin->get();

        if(rising) {
            if(packetStartCallback) {
                packetStartCallback();
            }
        } else {
            disableInt();
            // GDO0 is SYNC (in packet mode), so sync sequence was detected or sent
            if(packetEndCallback) {
                packetEndCallback(getState() == CC110L_STATE_RX);
            }

            if(getState() == CC110L_STATE_RX)
            {
                cometos::getScheduler().add(readTask, 0);
            }
            else if(getState() == CC110L_STATE_TX)
            {
                disableInt();
                enterIdle();
            }

            changeState(CC110L_STATE_IDLE);
            enableInt();
        }
    }
    else
    {
        disableInt();
        // GDO0 is RX FIFO threshold (in continuous mode), so readout FIFO
        cometos::getScheduler().add(readoutFIFOTask,0);
        enableInt();
    }
}

void CC110lApi::rxIdle(void)
{
    /* Disable pin interrupt */
    disableInt();
    /* Strobe IDLE */
    cmdStrobe(CC110L_SIDLE);

    /* Wait until chip is in IDLE */
    //while(cmdStrobe(CC110L_SNOP) & 0xF0);

    for(uint16_t i = 0; i < 1000; i++) {
        if(!(cmdStrobe(CC110L_SIDLE) & 0xF0)) {
	    break;
	}
    }

    if(cmdStrobe(CC110L_SNOP) & 0xF0) {
	// TODO error handling
	return;
    }

    /* Flush the Receive FIFO */
    cmdStrobe(CC110L_SFRX);

    /* Flush the Transmit FIFO */
    cmdStrobe(CC110L_SFTX);

    /* Clear pin interrupt flag */
    // clearIntFlag(); TODO why?
    return;
}

void CC110lApi::idleRx(void)
{
    //intFlag();
    cmdStrobe(CC110L_SRX);
    enableInt();
    return;
}

void CC110lApi::stateChangeConnect(Callback<void(ccStatus_t)> callback)
{
    this->stateChangeCallback = callback;
}

void CC110lApi::enableInt(void){
    gdo0pin->interruptActive(true);
    //EIMSK |= ((1 << INT7));//|(1 << INT5));     // Turns on INT4 and INT5
    /* DEBUG cometos::getCout() << "Interrupts enabled\n"; */
}

void CC110lApi::disableInt(void){
    gdo0pin->interruptActive(false);
    //EIMSK &= ~((1 << INT7));//|(1 << INT5));    // Turns off INT4 and INT5
    /* DEBUG cometos::getCout() << "Interrupts disabled\n"; */
}

void CC110lApi::printLastReceptionParameters() {
    getCout() << dec << "RSSI: " << (lastRSSI/10) << "." << (lastRSSI%10) << endl;
    getCout() << dec << "FREQEST: " << lastFREQEST << endl;
}

CC1190Api& CC110lApi::getCC1190() {
    return cc1190;
}

} /* namespace cometos */
