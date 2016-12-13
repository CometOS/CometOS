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

#include "DSMEPlatform.h"

#include "RFA1DriverLayer.h"
#include "DSMECcaLayer.h"
#include "openDSME/dsmeLayer/DSMELayer.h"
#include "MacSymbolCounter.h"

using namespace cometos;

namespace dsme {

uint8_t DSMEPlatform::data[127];
uint8_t DSMEPlatform::state = STATE_READY;
message_t DSMEPlatform::phy_msg;

Delegate<void(bool)> DSMEPlatform::txEndCallback;

uint32_t DSMEPlatform::getSymbolCounter() {
    return MacSymbolCounter::getInstance().getValue();
}

uint32_t DSMEPlatform::getSFDTimestamp() {
    // -2 since the AT86RF231 captures at the end of the PHR (+6us) instead at the end of the SFD as the ATmega256RFR2
    return MacSymbolCounter::getInstance().getCapture() - 2;
}

void DSMEPlatform::startTimer(uint32_t symbolCounterValue) {
    MacSymbolCounter::getInstance().setCompareMatch(symbolCounterValue);
}

bool DSMEPlatform::setChannelNumber(uint8_t channel) {
    this->channel = channel;
    cometos_error_t error = radioState_setChannel(this->channel);
    bool success = error != MAC_ERROR_BUSY;
    return success;
}

bool DSMEPlatform::startCCA() {
    return (cca_request() == MAC_SUCCESS);
}

bool DSMEPlatform::sendCopyNow(DSMEMessage* msg, Delegate<void(bool)> txEndCallback) {
    ASSERT(msg != nullptr);

    palExec_atomicBegin();
    {
        if (DSMEPlatform::state != STATE_READY) {
            palExec_atomicEnd();
            return false;
        }
        DSMEPlatform::state = STATE_SEND;
        DSMEPlatform::txEndCallback = txEndCallback;
    }
    palExec_atomicEnd();

    pktSize_t length = msg->getHeader().getSerializationLength() + msg->frame->getLength();

    /* maximum DSME payload size (IEEE 802.15.4e) - FCS length */
    ASSERT(length < 126);

    uint8_t *buffer = DSMEPlatform::data;

    /* serialize header */
    buffer << msg->getHeader();

    /* copy data */
    memcpy(buffer, msg->frame->getData(), msg->frame->getLength());

    phy_msg.data = DSMEPlatform::data;
    phy_msg.phyPayloadLen = length;
    phy_msg.requiresCca = false;

    /*
     * PHY-header (1 byte) + synchronization-header (1 byte)
     */
    phy_msg.phyFrameLen = length + 2;

    mac_result_t result = ccaSend_send(&phy_msg);

    if(result != MAC_SUCCESS) {
        DSMEPlatform::state = STATE_READY;
        return false;
    }
    else {
        return true;
    }
}

bool DSMEPlatform::sendDelayedAck(DSMEMessage *ackMsg, DSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback) {
    (void) receivedMsg;
    // the time is utilized anyway, so do not delay
    return sendCopyNow(ackMsg, txEndCallback);
}

/**
 * Interface to tosMac
 */
message_t* DSMEPlatform::receive_phy(message_t* phy_msg) {
    uint32_t sfdTimestamp = getSFDTimestamp();
    const uint8_t *buffer = phy_msg->data;

    dsme::DSMEMessage *msg = instance->getEmptyMessage();
    if (msg == nullptr) {
        /* '-> No space for a message could be allocated. */
        return phy_msg;
    }

    msg->startOfFrameDelimiterSymbolCounter = sfdTimestamp;

    /* deserialize header */
    bool success = msg->getHeader().deserializeFrom(buffer, phy_msg->phyPayloadLen);

    if(!success) {
        instance->releaseMessage(msg);
        return phy_msg;
    }

    if(msg->getHeader().getSrcAddrMode() == NO_ADDRESS && msg->getHeader().getFrameType() == IEEE802154eMACHeader::COMMAND) {
        for(int i = 0; i < phy_msg->phyPayloadLen; i++) {
            cometos::getCout() << "0x" << cometos::hex << phy_msg->data[i] << " ";
        }
        cometos::getCout() << cometos::endl;
	ASSERT(false);
    }

    /* copy data */
    ASSERT(phy_msg->phyPayloadLen >= msg->getHeader().getSerializationLength());
    msg->frame->setLength(phy_msg->phyPayloadLen - msg->getHeader().getSerializationLength());
    ASSERT(msg->frame->getLength() <= msg->frame->getMaxLength());
    memcpy(msg->frame->getData(), buffer, msg->frame->getLength());

    instance->dsme.getAckLayer().receive(msg);

    return phy_msg;
}

}

/**
 * Interface to tosMac
 */
void ccaSend_ready(mac_result_t error) {
    return;
}

/**
 * Interface to tosMac
 */
void ccaResult_ready(mac_result_t error) {
    dsme::DSMEPlatform::instance->getDSME().dispatchCCAResult(error == MAC_SUCCESS);
    return;
}


/**
 * Interface to tosMac
 */
void ccaSend_sendDone(message_t * msg, mac_result_t result) {
    ASSERT(dsme::DSMEPlatform::state == dsme::DSMEPlatform::STATE_SEND);

// TODO: schedule as task?
    dsme_atomicBegin();
    Delegate<void(bool)> cb = dsme::DSMEPlatform::txEndCallback;
    ASSERT(cb);
    dsme_atomicEnd();
    cb(result == MAC_SUCCESS);

    dsme::DSMEPlatform::state = dsme::DSMEPlatform::STATE_READY;
    return;
}

/**
 * Interface to tosMac
 */
message_t* radioReceive_receive(message_t* msg) {
    return dsme::DSMEPlatform::receive_phy(msg);
}

/**
 * Interface to tosMac
 */
void csmaRadioAlarm_fired() {
    /* Do nothing. */
    return;
}

void salRadioAlarm_fired() {
    /* Do nothing. */
    return;
}

/**
 * Interface to tosMac
 */
void tasklet_messageBufferLayer_run() {
    /* Do nothing. */
    return;
}
