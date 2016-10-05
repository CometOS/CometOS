/*
 * Copyright (c) 2007, Vanderbilt University
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
 * Author: Miklos Maroti
 * Author: Andreas Weigel (modifications for CometOS)
 */

#include <tasklet.h>
#include <stdint.h>
#include "cometos.h"
#include "SoftwareAckLayer.h"
#include "RFA1DriverLayer.h"
#include "RadioAlarm.h"
#include "tosUtil.h"
#include "CcaLayer.h"
#include "pinEventOutput.h"
#include "palLed.h"
/**
 * Port of tinyos software ack layer from the rfxlink library.
 */

//tasklet_norace
static uint8_t state;
enum
{
    STATE_READY = 0,
    STATE_DATA_SEND = 1,
    STATE_ACK_WAIT = 2,
    STATE_ACK_SEND = 3,
};

#define ACK_MSG_SIZE 3

//tasklet_norace
static message_t* txMsg;

static uint8_t ackBuf[ACK_MSG_SIZE];
//tasklet_norace
static message_t ackMsg;

static uint16_t ackTimeoutMicro = MAC_DEFAULT_ACK_WAIT_DURATION;

//static uint32_t tsReadyToAck;
//static uint32_t tsAckRcvd;
//static uint32_t tsAckToReady;

void sal_setAckWaitDuration(uint16_t durationUs) {
    ackTimeoutMicro = durationUs;
//    cometos::getCout() << cometos::dec << "ackWait=" << ackTimeoutMicro << cometos::endl;
}

uint16_t sal_getAckWaitDuration() {
    return ackTimeoutMicro;
}

//tasklet_async event
void ccaSend_ready()
{
    if( state == STATE_READY )
         salSend_ready();
}

mac_result_t sal_init() {
//    timer3_start();
    ackMsg.data = ackBuf;
    ackMsg.phyPayloadLen = ACK_MSG_SIZE;
    ackMsg.phyFrameLen = ACK_MSG_SIZE + 2;  //> FCF appended automatically
    return MAC_SUCCESS;
}

//tasklet_async command
mac_result_t salSend_send(message_t* msg)
{
//    tsAckToReady = 0;
//    tsReadyToAck = 0;
//    tsAckRcvd = 0;
    mac_result_t error;

    if( state == STATE_READY )
    {
        if( (error = ccaSend_send(msg)) == MAC_SUCCESS )
        {
//            AckReceivedFlag_clear(msg);
            tosUtil_clearAckReceivedFlag(msg);
            state = STATE_DATA_SEND;
            txMsg = msg;
        }
    }
    else {
        error = MAC_ERROR_BUSY;
    }

    return error;
}

//tasklet_async event
void ccaSend_sendDone(message_t* msg, mac_result_t error)
{
    if( state == STATE_ACK_SEND )
    {
        // TODO: what if error != SUCCESS
        ASSERT( error == MAC_SUCCESS );

        state = STATE_READY;
    }
    else
    {
        ASSERT( state == STATE_DATA_SEND );
        ASSERT( radioAlarm_isFree() );

        if( error == MAC_SUCCESS && tosUtil_ackRequested(txMsg) && radioAlarm_isFree() ) {
            radioAlarm_wait(ackTimeoutMicro>>4, SAL_ID);
            state = STATE_ACK_WAIT;
//            tsReadyToAck = timer3_get();
        } else {
            state = STATE_READY;
            salSend_sendDone(txMsg, error);
        }
    }
}

//tasklet_async event
void salRadioAlarm_fired() {
    ASSERT( state == STATE_ACK_WAIT );
//    Config_reportChannelError();
//    tsAckToReady = timer3_get();
    txMsg->wasAcked = false;
    state = STATE_READY;
    salSend_sendDone(txMsg, MAC_SUCCESS); // we have sent it, but not acked
}

//tasklet_async event
// superfluous for non-SPI-based radio
//bool RadioReceive_header(message_t* msg)
//{
//    // drop unexpected ACKs
//    if( tosUtil_isAckFrame(msg) ) {
//        return state == STATE_ACK_WAIT;
//    }
//
//
//    // drop packets that need ACKs while waiting for our ACK
////      if( state == STATE_ACK_WAIT && Config.requiresAckWait(msg) )
////          return FALSE;
//
//    return  RadioReceive_header(msg);
//}

//tasklet_async event
message_t* radioReceive_receive(message_t* msg) {
    ASSERT( state == STATE_ACK_WAIT || state == STATE_READY );

    if( tosUtil_isAckFrame(msg) )
    {
//        tsAckRcvd = timer3_get();
//        cometos::getCout() << (int) state << "|" << (int) txMsg->data[0] << "|" << (int) txMsg->data[2] << "|"
//                << (int) msg->data[0] << "|" << (int) msg->data[2] << "|"
//                << tsReadyToAck << "|" << tsAckToReady << "|" << tsAckRcvd << cometos::endl;
        if( state == STATE_ACK_WAIT && tosUtil_verifyAck(txMsg, msg) )
        {
            radioAlarm_cancel();
//            AckReceivedFlag_set(txMsg);
            tosUtil_setAckReceivedFlag(txMsg);

            state = STATE_READY;
            salSend_sendDone(txMsg, MAC_SUCCESS);
        } else {
            EVENT_OUTPUT_WRITE(PEO_RX_DROPPED);
        }

        return msg;
    }

    if( state == STATE_READY && tosUtil_requiresAckReply(msg) )
    {
        tosUtil_createAckFrame(msg, &ackMsg);
        // TODO: what to do if we are busy and cannot send an ack?
        //       we cannot really prevent situations, where we receive another
        //       frame directly after reception ended, so we just ignore
        //       this case, it should not happen too often
        if( ccaSend_send(&ackMsg) == MAC_SUCCESS ) {
            state = STATE_ACK_SEND;
        } else {
//            ASSERT(false);
        }
    }

    return salReceive_receive(msg);
}

///*----------------- PacketAcknowledgements -----------------*/
//
////async command
//mac_result_t PacketAcknowledgements_requestAck(message_t* msg)
//{
//    Config_setAckRequired(msg, TRUE);
//
//    return SUCCESS;
//}
//
////async command
//mac_result_t PacketAcknowledgements_noAck(message_t* msg)
//{
//    Config_setAckRequired(msg, FALSE);
//
//    return SUCCESS;
//}
//
////async command
//bool PacketAcknowledgements_wasAcked(message_t* msg)
//{
//    return AckReceivedFlag_get(msg);
//}
