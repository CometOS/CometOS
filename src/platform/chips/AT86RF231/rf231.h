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

#ifndef RF231_H
#define RF231_H

#include "IGPIOPin.h"
#include "palSpi.h"
#include "at86rf231_registers.h"
#include "palTimer.h"
#include "PhyCount.h"

#define RADIO_TX_BUFFER_SIZE 3  //only enough buffer needed for commands and results
#define RADIO_RX_BUFFER_SIZE 3  //payload is directly written to rxMsg

/* FBEI needs 750ns to be valid
 * each loop takes at least 3 instructions:
 * nop, jump to head, compare
 * 750 ns are guaranteed, if clock_tick_time * 3* WAIT_TICKS_FBEI_VALID > 750
 *
 */
#define CLOCK_TICK_TIME_NS 14
#define WAIT_TICKS_FBEI_VALID 750 / (3 * CLOCK_TICK_TIME_NS) + 1
#define MICRO_SECONDS_FOR_ONE_BYTE 32

typedef void (*callback_t)(void);

/**
 * This Function is called by the timer during parallel
 * download of a frame. It triggers a download of the next
 * byte, if available.
 */
void signalNextByteAvailable(void);

extern cometos::PhyCounts pc;

namespace cometos {

class Rf231 {

private:

	//PINS used by the transceiver
    IGPIOPin* rst_pin;
    IGPIOPin* slptr_pin;
    IGPIOPin* irq_pin;

    PalSpi* spi;
    PalTimer* timer;

    uint8_t txBuffer[RADIO_TX_BUFFER_SIZE];
    uint8_t rxBuffer[RADIO_RX_BUFFER_SIZE];

    //for parallel reception of a frame during rx
    volatile bool parallelReceptionOn;
    volatile bool receptionFinished;
    uint8_t failedTries;
    uint8_t frameLength, frameMissing;
    uint8_t *dataPtr, *origDataPtr, *lqiPtr;
    uint8_t maxBufferLength;

    //Callbacks
    callback_t interruptCallback, frameDownloadCallback, frameTimeoutCallback;

    Rf231(){
    	lqiPtr = NULL;
    	dataPtr = NULL;
        origDataPtr = NULL;
    	slptr_pin = NULL;

        interruptCallback = NULL;

        frameDownloadCallback = NULL;
        frameTimeoutCallback = NULL;
    }

    bool pendingPDTDis;
    bool abort;

public:

    /**
     * Sets the bus and pins, that the Microcontroller uses to interface with the RF231
     * transceiver.
     *
     * @param spi PalSpi implementation, initialized in Master Mode
     * @param rst Reset pin
     * @param slptr Multipurpuse pin of transceiver
     * @param irq IRQ Pin (used as Frame Buffer Empty Indicator), Interrupt must be configured manually.
     * @param timer PalTimer implementation.
     */
    void init(PalSpi* spi, IGPIOPin* rst, IGPIOPin* slptr, IGPIOPin* irq, PalTimer* timer) {

        rst_pin = rst;
        slptr_pin = slptr;
        irq_pin = irq;
        this->spi = spi;
        this->timer = timer;

        timer->setFrequency(1e6); // us

        //Init pins
        rst_pin->setDirection(IGPIOPin::Direction::OUT);
        slptr_pin->setDirection(IGPIOPin::Direction::OUT);
        irq_pin->setDirection(IGPIOPin::Direction::IN);

        parallelReceptionOn = false;
        receptionFinished = false;

        // zeroByte for receive only transmissions
        txBuffer[2] = 0;

        reset();
    }

    /**
     * Hard resets the transceiver and sets the interface Pins of
     * the microcontroller to default values.
     * Waits until the Transceiver is in READY State.
     */
    void reset() {
        pendingPDTDis = false;
        abort = false;

        //reset to default pin values
        spi->enable();
    	spi->disable();
        rst_pin->clear();
        slptr_pin->clear();

        //wait a millisecond
        timer->delay(AT86RF231_TIMING__RESET);

        //release reset
        rst_pin->set();

        timer->delay(AT86RF231_TIMING__RESET_TO_TRX_OFF);

        //wait until status is ready
        uint8_t status;
        do {
            status = readRegister(AT86RF231_REG_TRX_STATUS);
            status &= AT86RF231_TRX_STATUS_MASK_TRX_STATUS;
        } while (status == AT86RF231_TRX_STATUS_STATE_TRANS_IN_PROGRESS);
    }

    /**
     * Reads a single register at a given Address
     *
     * @param address Is the address of the register in the transceivers memory.
     *
     * @return The content of the register at given address.
     */
    uint8_t readRegister(uint8_t address) {

        uint8_t txBuffer[2], rxBuffer[2];

        //read register command: 10 regAddress
        txBuffer[0] = AT86RF231_ACCESS_REG | AT86RF231_ACCESS_READ | address;
        txBuffer[1] = 0;

        palExec_atomicBegin();
        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);
        disableSPI();
        palExec_atomicEnd();

        return rxBuffer[1];
    }

    /**
     * Writes a value to a register at given address.
     */
    void writeRegister(uint8_t address, uint8_t value){
        txBuffer[0] = AT86RF231_ACCESS_REG | AT86RF231_ACCESS_WRITE | address;
        txBuffer[1] = value;

        palExec_atomicBegin();
        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);
        disableSPI();
        palExec_atomicEnd();
    }

    /**
     * Reads the content of the frame buffer.
     *
     * @param length Address of length variable. The frame length will be written to this address.
     * @param data_buffer The frame content will be written to the memory starting at this address.
     * @param rssi The RSSI value, this package was received with will be written to this address.
     */
    uint8_t readFrameBuffer(uint8_t* length, uint8_t* data_buffer, uint8_t max_buffer_length, uint8_t* rssi) {
        // read header information from rf2xx
        txBuffer[0] = AT86RF231_ACCESS_FRAMEBUFFER | AT86RF231_ACCESS_READ;
        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);

        //header contains no of bytes per frame -> load rest
        *length = rxBuffer[1];

        if (*length >= 128) {
            disableSPI();
        	return COMETOS_ERROR_FAIL;
        }

        // read rest of frame
        uint16_t i;
        uint8_t result = COMETOS_SUCCESS;
        for (i = 0; i < *length; i++){
            if(abort) {
                result = COMETOS_ERROR_FAIL;
                break;
            }
            ASSERT(i < max_buffer_length);
            spi->transmitBlocking(txBuffer + 2, data_buffer + i, 1);
        }

        // byte after payload contains quality information
        if(!abort) {
            spi->transmitBlocking(txBuffer + 2, rssi, 1);
        }
        else {
            result = COMETOS_ERROR_FAIL;
        }
        disableSPI();

        return result;
    }

private:
    void enterPDTDis() {
        abort = false;
        pendingPDTDis = false;
        
        // The methods will call disableSPI recursively, but since pendingPDTDis = false
        // and we are inside an atomic block, we will not go here again.
	    uint8_t regVal = readRegister(AT86RF231_REG_RX_SYN);
	    writeRegister(AT86RF231_REG_RX_SYN, regVal | AT86RF231_RX_SYN_PDT_DIS_MASK);
    }

public:
    void disableSPI() {
        palExec_atomicBegin();
        spi->disable();
        if(pendingPDTDis) {
            enterPDTDis();
        }
        palExec_atomicEnd();
    }

    void forcePDTDis() {
        palExec_atomicBegin();
        if(!spi->isAvailable()) {
            abort = true;
            pendingPDTDis = true;
        }
        else {
            enterPDTDis();
        }
        palExec_atomicEnd();
    }

    /**
     * Writes the content of a buffer to the transceivers frame buffer.
     *
     * @param payload The start address of the source buffer
     * @param length The number of bytes to transmit to the transceiver.
     */
    uint8_t writeFrameBuffer(uint8_t* payload, uint8_t length) {

    	if (length >= 128)
    		return COMETOS_ERROR_FAIL;

        //write command and header in buffer
        txBuffer[0] = AT86RF231_ACCESS_FRAMEBUFFER | AT86RF231_ACCESS_WRITE;
        txBuffer[1] = length;

        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);

        //trigger tx-start
        slptr_pin->set();

        //transmit rest
        uint16_t i;
        uint8_t result = COMETOS_SUCCESS;
        for (i = 0; i < length; i++){
            if(abort) {
                result = COMETOS_ERROR_FAIL;
                break;
            }
            spi->transmitBlocking(payload + i, rxBuffer, 1);
        }
        disableSPI();

        slptr_pin->clear();

        return result;
    }

    /**
     * Starts a asynchronous download from the transceivers frame buffer.
     *
     * @param data Start address of the target buffer.
     * @param length Target variable to write the length of the frame to.
     * @param lqi Target variable to write the link quality indicator to.
     * @param success_callback Callback function that is called when the whole frame has
     * 		successfully recevied.
     * @param timeout_callback Callback function called, when the frame could not successfully be
     * 		transmitted to the buffer.
     */
    uint8_t downloadFrameParallel(uint8_t *data,  uint8_t max_buffer_length, uint8_t* length, uint8_t* lqi, callback_t success_callback, callback_t timeout_callback) {

    	parallelReceptionOn = true;

    	//when interrupt is incoming, PHR is already valid, start downloading
        txBuffer[0] = AT86RF231_ACCESS_FRAMEBUFFER | AT86RF231_ACCESS_READ;

        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);

        *length = rxBuffer[1];
        frameLength = rxBuffer[1];
        frameMissing = rxBuffer[1];
        frameDownloadCallback = success_callback;
        frameTimeoutCallback = timeout_callback;
        dataPtr = data;
        origDataPtr = data;
        maxBufferLength = max_buffer_length;
        lqiPtr = lqi;

        if (frameLength > 128 || abort) {
        	parallelReceptionOn = false;
            disableSPI();
        	return COMETOS_ERROR_FAIL;
        }

        //Wait 750ns to ensure valid irq pin state.
        int j;
        for (j = 0; j < WAIT_TICKS_FBEI_VALID; j++)
        	__asm("nop");

        failedTries = 0;
        nextByteAvailable();

        return COMETOS_SUCCESS;
    }

    /**
     * Checks, if the Frame Buffer Empty Indicator signals an available byte and
     * downloads it, if available.
     */
    void nextByteAvailable() {
    	timer->stop();
    	uint8_t txBuffer = 0;

    	if (irq_pin->get() != 0 || abort) {
    		failedTries++;

            if (failedTries >= 3 || abort) {
                pc.numTo++;
                parallelReceptionOn = false;
                disableSPI();

                if (frameTimeoutCallback) {
                    frameTimeoutCallback();
                }
                return;
            }
    	}

    	while (irq_pin->get() == 0) {
    		failedTries = 0;
			ASSERT(dataPtr-origDataPtr < maxBufferLength);
			spi->transmitBlocking(&txBuffer, dataPtr++, 1);
			frameMissing--;

			if (frameMissing == 0) {
				parallelReceptionOn = false;
				receptionFinished = true;
				spi->transmitBlocking(&txBuffer, lqiPtr, 1);
                disableSPI();

				if (frameDownloadCallback)
					frameDownloadCallback();

				return;
			}

	        //Wait 750ns to ensure valid irq pin state.
	        int j;
	        for (j = 0; j < WAIT_TICKS_FBEI_VALID; j++)
	        	__asm("nop");

            if(abort) {
                parallelReceptionOn = false;
                disableSPI();
                if (frameTimeoutCallback) {
                    frameTimeoutCallback();
                }
                return;
            }

    	}

    	timer->start_async(MICRO_SECONDS_FOR_ONE_BYTE, signalNextByteAvailable);
    }


    /**
     * Transfers a part of the transceivers SRAM to a buffer.
     *
     * @param address Is the start address in the SRAM of the transceiver
     * @param length The number of bytes to transmit
     * @param result The start address of the buffer in the microcontrollers memory.
     */
    uint8_t readSram(uint8_t address, uint8_t length, uint8_t* result) {

        //address must be smaller or equal 0x7F

        //write command and header in buffer
        txBuffer[0] = AT86RF231_ACCESS_SRAM | AT86RF231_ACCESS_READ;
        txBuffer[1] = address;

        spi->enable();

        spi->transmitBlocking(txBuffer, rxBuffer, 2);

        uint16_t i;
        uint8_t resultCode = COMETOS_SUCCESS;
        for (i = 0; i < length; i++){
            if(abort) {
                resultCode = COMETOS_ERROR_FAIL;
                break;
            }
            spi->transmitBlocking(txBuffer + 2, result + i, 1);
        }

        disableSPI();

        return resultCode;
    }

    /**
     * Transfers bytes from a buffer on the microcontroller to the
     * transceivers SRAM
     *
     * @param address Start address in the transceivers memory
     * @param data Pointer to the source buffer in the microcontrollers memory
     * @param length Number of bytes to transmit
     */
    uint8_t writeSram(uint8_t address, uint8_t *data, uint8_t length) {
        txBuffer[0] = AT86RF231_ACCESS_SRAM | AT86RF231_ACCESS_READ;
        txBuffer[1] = address;

        spi->enable();
        spi->transmitBlocking(txBuffer, rxBuffer, 2);

        uint8_t i;
        uint8_t resultCode = COMETOS_SUCCESS;
        for(i = 0; i < length; i++){
            if(abort) {
                resultCode = COMETOS_ERROR_FAIL;
                break;
            }
            spi->transmitBlocking(data + i, rxBuffer, 1 );
        }

        disableSPI();

        return resultCode;
    }

    /**
     * Reads the status reagister and returns the transceivers status.
     *
     * @return TRX Status, encoding can be found at at86rf231_registers.h
     */
    inline uint8_t getRfStatus() {
        uint8_t status = readRegister(AT86RF231_REG_TRX_STATUS);
        status &= AT86RF231_TRX_STATUS_MASK_TRX_STATUS;
        return status;
    }

    /**
     * Sends a state change command to the transceiver
     *
     * @param state The commanded state encoded as in at86rf231_registers.h
     */
    inline void cmd_state(uint8_t state) {
        uint8_t reg = readRegister(AT86RF231_REG_TRX_STATE);
        reg &= ~ 0x1f;
        reg |= (state & 0x1f);
        writeRegister(AT86RF231_REG_TRX_STATE, reg);
    }

    /**
     * Sets or clears the slpTr pin. This is a multipurpose pin, but in active
     * state it can be used to start a transmission. See Rf231 Datasheet
     * for more information.
     *
     * @param value The value to set the pin to.
     */
    inline void setSlpTr(bool value){
        if (value){
            slptr_pin->set();
        }
        else {
            slptr_pin->clear();
        }
    }

    /**
     * Shows current status of a asynchronous parallel
     * framebuffer download.
     *
     * @return True, if currently a frame buffer transfer
     * 		is active in the background, false else.
     */
    inline bool downloadPending() {
    	return parallelReceptionOn;
    }

    /**
     * Shows, that a download has been finished, but not
     * acknowledged (used) by the caller. See also @a downloadRetrieved().
     *
     * @return True, if a completed download has not been acknowledged, False else.
     */
    inline bool downloadFinished() {
        bool isFinished;
        palExec_atomicBegin();
        isFinished = receptionFinished;
        palExec_atomicEnd();
        return isFinished;
    }

    /**
     * Acknowledges a finished download. The caller can use
     * this function to flag, that a completed download
     * has already been noticed.
     */
    inline void downloadRetrieved() {
        receptionFinished = false;
    }

    /**
     * @return True, if the SPI Bus is currently not used
     * for any pending transfer, false else.
     */
    inline bool spiAvailable() {
    	return spi->isAvailable();
    }

    /**
     * Sets a callback function that should be called whenever
     * the transceiver signals an interrupt.
     *
     * Relaying the interrupts over this class seems complex
     * at first, but makes sense, since the interrupt pin can
     * be used as FrameBufferEmptyIndicator during frame down-
     * load. This logic is also implemented in @triggerInterrupt.
     * When the application using the transceiver would configure
     * the interrupt handling directly, the FBEI logic would also
     * have to be implemented anew.
     */
    inline void setInterruptCallback(callback_t c){
        interruptCallback = c;
    }

    /**
     * Notifies the class of an interrupt. Setup of the
     * Interrupt pin is deliberately not directly implemented
     * here, since the class should be independend of platform
     * details.
     */
    inline void triggerInterrupt(){

        // During reception the functionality of the irq-pin is switched
        // to indicate validity of next byte in frame buffer
        if (parallelReceptionOn){
        	return;
        }

        if (interruptCallback != NULL){
            interruptCallback();
        }
    }

    /**
     * Return an instance of this class. Singleton pattern is
     * enforced.
     */
    static Rf231* getInstance() {
        static Rf231 singleton;
        return &singleton;
    }
};

}



#endif //RF231_H
