/*
 * Copyright (c) 2010, University of Szeged
 * Copyright (c) 2010, Aarhus Universitet
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Miklos Maroti,
 * Author: Morten Tranberg Hansen
 * Author: Andreas Weigel (modifications for CometOS)
 */

#include "PacketLinkLayer.h"
#include <stdint.h>
#include "logging.h"
#include "SoftwareAckLayer.h"
#include "RFA1DriverLayer.h"
#include "RadioAlarm.h"
#include "tosUtil.h"
#include "MessageBufferLayer.h"
#include "TaskScheduler.h"
#include "cometos.h"
#include "pinEventOutput.h"

//generic module PacketLinkLayerP()
//{
//    provides
//    {
//        interface BareSend as Send;
//        interface PacketLink;
//        interface RadioPacket;
//    }
//
//    uses
//    {
//        interface BareSend as csmaSend_
//        interface PacketAcknowledgements;
//        interface Timer<TMilli> as DelayTimer;
//        interface RadioPacket as SubPacket;
//    }
//}

void delayTimer_fired();
void send();

enum
{
    STATE_READY = 0,
    STATE_SENDING = 1,
    STATE_SENDDONE = 2,
    STATE_SIGNAL = 4,   // add error code
};

static uint8_t pllState = STATE_READY;
static message_t *currentMsg;
static uint8_t maxTxAttempts = MAC_DEFAULT_FRAME_RETRIES + 1;

void mac_getAutoAckConfig(mac_ackCfg_t *cfg);

mac_result_t mac_setAutoAckConfig(const mac_ackCfg_t *cfg) {
    if (cfg->maxFrameRetries >= 0 && cfg->maxFrameRetries <= 15) {
        sal_setAckWaitDuration(cfg->ackWaitDuration);
        maxTxAttempts = cfg->maxFrameRetries + 1;
        return MAC_SUCCESS;
    } else {
        return MAC_ERROR_SIZE;
    }
}

void mac_getAutoAckConfig(mac_ackCfg_t* cfg) {
    cfg->ackWaitDuration = sal_getAckWaitDuration();
    cfg->maxFrameRetries = maxTxAttempts - 1;
}


cometos::SimpleTask delayTimer(delayTimer_fired);
cometos::SimpleTask sendTask(send);

/**
 * We do everything from a single task in order to call csmaSend_send
 * and pllSend_sendDone only once. This helps inlining the code and
 * reduces the code size.
implementation
{    */
//task
void send()
{
    uint16_t txAttempts = 0; // 0 denotes no ACK expected, e.g., for broadcasts

    ASSERT( pllState != STATE_READY );

    if (tosUtil_ackRequested(currentMsg)) {
        txAttempts = maxTxAttempts;
    }
//    txAttempts = PacketLink.getRetries(currentMsg);

    if( pllState == STATE_SENDDONE )
    {
        if( txAttempts == 0 || tosUtil_wasMsgAcked(currentMsg) ) {  //PacketAcknowledgements.wasAcked(currentMsg) )
            pllState = STATE_SIGNAL + MAC_SUCCESS;
            EVENT_OUTPUT_WRITE(PEO_TX_SUCCESS);
        }
        // note, after the first transmission, we do the first retry, yielding
        // the second txAttempt; therefore, txAttempts has to be larger
        // than numRetransmissions + 1 here to trigger a new (re)transmission
        else if( (currentMsg->txInfo.numRetransmissions + 1u) < txAttempts )
        {
            uint16_t delay;
            // we retransmit the frame again
            currentMsg->txInfo.numRetransmissions += 1;
            pllState = STATE_SENDING;
            delay = tosUtil_getRetryDelay(currentMsg); //PacketLink.getRetryDelay(currentMsg);
            EVENT_OUTPUT_WRITE(PEO_TX_RETRY);
            if( delay > 0 )
            {
//                DelayTimer.startOneShot(delay);
                cometos::getScheduler().add(delayTimer, delay);
                return;
            }
        }
        else {
            EVENT_OUTPUT_WRITE(PEO_TX_FAIL);
            pllState = STATE_SIGNAL + MAC_ERROR_FAIL;
        }
    }

    if( pllState == STATE_SENDING )
    {
        pllState = STATE_SENDDONE;
        tosUtil_clearAckReceivedFlag(currentMsg);
        if( mblSend_send(currentMsg) != MAC_SUCCESS ) {
            //post
            cometos::getScheduler().add(sendTask);
        }
        return;
    }

    if( pllState >= STATE_SIGNAL )
    {
        mac_result_t error = pllState - STATE_SIGNAL;

        // is now handled directly via the txInfo attached to the message
        // do not update the retries count for non packet link messages
//        if( txAttempts > 0 ) {
//            currentMsg->txInfo.numRetransmissions = totalRetries;
////            PacketLink.setRetries(currentMsg, totalRetries);
//        }

        pllState = STATE_READY;
        pllSend_sendDone(currentMsg, error);
    }
}

//event
void mblSend_sendDone(message_t* msg, mac_result_t error)
{
    ASSERT( pllState == STATE_SENDDONE || pllState == STATE_SIGNAL + MAC_ERROR_CANCEL );
    ASSERT( msg == currentMsg );

    if( error != MAC_SUCCESS ) {
        pllState = STATE_SIGNAL + error;
        if (error == MAC_ERROR_BUSY) {
            EVENT_OUTPUT_WRITE(PEO_TX_BO_FAIL);
        }
    }

    //post
    cometos::getScheduler().add(sendTask);
}

//event
void delayTimer_fired()
{
    ASSERT( pllState == STATE_SENDING );

//    post
    cometos::getScheduler().add(sendTask);
}

//command
mac_result_t pllSend_send(message_t *msg)
{
    if( pllState != STATE_READY ) {
        return MAC_ERROR_BUSY;
    }
//    EVENT_OUTPUT_WRITE(PEO_TX_REQUEST);
//    // it is enough to set it only once
//    if( PacketLink.getRetries(msg) > 0 ) {
//        PacketAcknowledgements.requestAck(msg);
//    }

    currentMsg = msg;
    currentMsg->txInfo.numRetransmissions = 0;
    currentMsg->txInfo.numBackoffs = 0;
    pllState = STATE_SENDING;
//    post
    cometos::getScheduler().add(sendTask);

    return MAC_SUCCESS;
}

//command
mac_result_t pllSend_cancel(message_t *msg)
{
    if( currentMsg != msg || pllState == STATE_READY )
        return MAC_ERROR_FAIL;

    // if a send is in progress
    if( pllState == STATE_SENDDONE ) {
        mblSend_cancel(msg);
    } else {
//        post
        cometos::getScheduler().add(sendTask);
    }


//    DelayTimer.stop();
    pllState = STATE_SIGNAL + MAC_ERROR_CANCEL;

    return MAC_SUCCESS;
}

//// ------- PacketLink
//
//link_metadata_t* getMeta(message_t* msg)
//{
//    return ((void*)msg) + sizeof(message_t) - RadioPacket.metadataLength(msg);
//}
//
////command
//void PacketLink.setRetries(message_t *msg, uint16_t maxTxAttempts)
//{
//    getMeta(msg)->maxTxAttempts = maxTxAttempts;
//}
//
////command
//void PacketLink.setRetryDelay(message_t *msg, uint16_t retryDelay)
//{
//    getMeta(msg)->retryDelay = retryDelay;
//}
//
////command
//uint16_t PacketLink.getRetries(message_t *msg)
//{
//    return getMeta(msg)->maxTxAttempts;
//}
//
////command
//uint16_t PacketLink.getRetryDelay(message_t *msg)
//{
//    return getMeta(msg)->retryDelay;
//}
//
////command
//bool PacketLink.wasDelivered(message_t *msg)
//{
//    return PacketAcknowledgements.wasAcked(msg);
//}
//
//// ------- RadioPacket
//
////async command
//uint8_t RadioPacket.headerLength(message_t* msg)
//{
//    return SubPacket.headerLength(msg);
//}
//
////async command
//uint8_t RadioPacket.payloadLength(message_t* msg)
//{
//    return SubPacket.payloadLength(msg);
//}
//
////async command
//void RadioPacket.setPayloadLength(message_t* msg, uint8_t length)
//{
//    SubPacket.setPayloadLength(msg, length);
//}
//
//async command uint8_t RadioPacket.maxPayloadLength()
//{
//    return SubPacket.maxPayloadLength();
//}
//
//async command uint8_t RadioPacket.metadataLength(message_t* msg)
//{
//    return SubPacket.metadataLength(msg) + sizeof(link_metadata_t);
//}
//
//async command void RadioPacket.clear(message_t* msg)
//{
//    getMeta(msg)->maxTxAttempts = 0;
//    SubPacket.clear(msg);
//}

