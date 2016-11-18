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
 *         Andreas Huber <huberan@ee.ethz.ch>
 * Author: Andreas Weigel (modifications for CometOS)
 */

#include "MessageBufferLayer.h"
#include "tasklet.h"
#include <stdint.h>
#include "logging.h"
#include "SoftwareAckLayer.h"
#include "RFA1DriverLayer.h"
#include "CsmaLayer.h"
#include "RadioAlarm.h"
#include "tosUtil.h"
#include "CcaLayer.h"
#include "PacketLinkLayer.h"
#include "palId.h"
#include "cometos.h"
#include "palExec.h"
#include "OutputStream.h"
#include "PhyCount.h"

#include "tasklet.h"
#include "logging.h"

//generic module MessageBufferLayerP()
//{
//    provides
//    {
//        interface SplitControl;
//        interface Init as SoftwareInit;
//
//        interface BareSend as Send;
//        interface BareReceive as Receive;
//        interface RadioChannel;
//    }
//    uses
//    {
//        interface RadioState;
//        interface Tasklet;
//        interface RadioSend;
//        interface RadioReceive;
//    }
//}

/*----------------- State -----------------*/

//tasklet_norace
uint8_t mblState;
enum
{
    STATE_READY = 0,
    STATE_TX_PENDING = 1,
    STATE_TX_RETRY = 2,
    STATE_TX_SEND = 3,
    STATE_TX_DONE = 4,
    STATE_TURN_ON = 5,
    STATE_TURN_OFF = 6,
    STATE_CHANNEL = 7,
};

extern cometos::PhyCounts pc;

//command
//mac_result_t SplitControl.start()
//{
//    mac_result_t error;
//
//    tasklet_suspend();
//
//    if( mblState != STATE_READY )
//        error = MAC_ERROR_BUSY;
//    else
//    {
//        error = RadioState.turnOn();
//
//        if( error == MAC_SUCCESS )
//            mblState = STATE_TURN_ON;
//    }
//
//    tasklet_resume();
//
//    return error;
//}
//
////command
//mac_result_t SplitControl.stop()
//{
//    mac_result_t error;
//
//    tasklet_suspend();
//
//    if( mblState != STATE_READY )
//        error = MAC_ERROR_BUSY;
//    else
//    {
//        error = RadioState.turnOff();
//
//        if( error == MAC_SUCCESS )
//            mblState = STATE_TURN_OFF;
//    }
//
//    tasklet_resume();
//
//    return error;
//}

//command mac_result_t RadioChannel.setChannel(uint8_t channel)
//{
//    mac_result_t error;
//
//    tasklet_suspend();
//
//    if( mblState != STATE_READY )
//        error = MAC_ERROR_BUSY;
//    else
//    {
//        error = RadioState.setChannel(channel);
//
//        if( error == MAC_SUCCESS )
//            mblState = STATE_CHANNEL;
//    }
//
//    tasklet_resume();
//
//    return error;
//}
//
//command uint8_t RadioChannel.getChannel()
//{
//    return RadioState.getChannel();
//}

//task void mblStateDoneTask()
//{
//    uint8_t s;
//
//    s = mblState;
//
//    // change the mblState before so we can be reentered from the event
//    mblState = STATE_READY;
//
//    if( s == STATE_TURN_ON )
//        SplitControl.startDone(MAC_SUCCESS);
//    else if( s == STATE_TURN_OFF )
//        SplitControl.stopDone(MAC_SUCCESS);
//    else if( s == STATE_CHANNEL )
//        RadioChannel.setChannelDone();
//    else    // not our event, ignore it
//        mblState = s;
//}

//tasklet_async event void RadioState.done()
//{
//    post mblStateDoneTask();
//}

//default event void SplitControl.startDone(mac_result_t error)
//{
//}
//
//default event void SplitControl.stopDone(mac_result_t error)
//{
//}

//default event void RadioChannel.setChannelDone()
//{
//}

/*----------------- Send -----------------*/
static void mblSend();

static void mblDeliver();

static

cometos::SimpleTask sendTask(mblSend);
cometos::SimpleTask deliverTask(mblDeliver);

message_t* mblTxMsg;
//tasklet_norace
mac_result_t txError;
uint8_t retries;

// Many MAC_ERROR_BUSY replies from RadioSend are normal if the channel is congested
enum { MAX_RETRIES = 1 };

//task
static void mblSend()
{
    bool done = false;

    tasklet_suspend();

    ASSERT( mblState == STATE_TX_PENDING || mblState == STATE_TX_DONE );

    if( mblState == STATE_TX_PENDING && ++retries <= MAX_RETRIES )
    {
        txError = csmaSend_send(mblTxMsg);
        if( txError == MAC_SUCCESS ) {
            mblState = STATE_TX_SEND;
        } else {
            mblState = STATE_TX_RETRY;
        }
    }
    else
    {
        mblState = STATE_READY;
        done = true;
    }

    tasklet_resume();

    if( done ) {
        mblSend_sendDone(mblTxMsg, txError);
    }
}

//tasklet_async event
void csmaSend_sendDone(message_t* msg, mac_result_t result)
{
    ASSERT( mblState == STATE_TX_SEND );
    ASSERT( msg == mblTxMsg);

    txError = result;
    if( result == MAC_SUCCESS )
        mblState = STATE_TX_DONE;
    else
        mblState = STATE_TX_PENDING;

    //post
    cometos::getScheduler().add(sendTask);
}

//command
mac_result_t mblSend_send(message_t* msg)
{
    mac_result_t result;

    tasklet_suspend();

    if( mblState != STATE_READY ) {
        result = MAC_ERROR_BUSY;
    } else {
        mblTxMsg = msg;
        mblState = STATE_TX_PENDING;
        retries = 0;

//        post
        cometos::getScheduler().add(sendTask);
        result = MAC_SUCCESS;
    }

    tasklet_resume();

    return result;
}

//tasklet_async event
void csmaSend_ready()
{
    if( mblState == STATE_TX_RETRY )
    {
        mblState = STATE_TX_PENDING;
//        post
        cometos::getScheduler().add(sendTask);
    }
}

//tasklet_async event
void tasklet_messageBufferLayer_run()
{
}

//command
mac_result_t mblSend_cancel(message_t* msg)
{
    mac_result_t result;

    tasklet_suspend();

    ASSERT( msg == mblTxMsg );

    if( mblState == STATE_TX_PENDING || mblState == STATE_TX_RETRY )
    {
        mblState = STATE_TX_DONE;
        txError = MAC_ERROR_CANCEL;
        result = MAC_SUCCESS;

        //post
        cometos::getScheduler().add(sendTask);
    }
    else
        result = MAC_ERROR_BUSY;

    tasklet_resume();

    return result;
}

/*----------------- Receive -----------------*/

enum
{
    RECEIVE_QUEUE_SIZE = 1,
};

uint8_t messageData[RECEIVE_QUEUE_SIZE][MAC_MAX_PAYLOAD_SIZE + MAC_HEADER_SIZE];
message_t receiveQueueData[RECEIVE_QUEUE_SIZE];
message_t* receiveQueue[RECEIVE_QUEUE_SIZE];

uint8_t receiveQueueHead;
uint8_t receiveQueueSize;

//command
mac_result_t mbl_init()
{
    uint8_t i;

    for(i = 0; i < RECEIVE_QUEUE_SIZE; ++i) {
        receiveQueue[i] = receiveQueueData + i;
        receiveQueue[i]->data = messageData[i];
    }

    return MAC_SUCCESS;
}

//tasklet_async event bool RadioReceive.header(message_t* msg)
//{
//    bool notFull;
//
//    // this prevents undeliverable messages to be acknowledged
//    atomic notFull = receiveQueueSize < RECEIVE_QUEUE_SIZE;
//
//    return notFull;
//}

//task
void mblDeliver()
{
    // get rid of as many messages as possible without intervening tasks
    for(;;)
    {
        message_t* msg;

        palExec_atomicBegin();
        {
            if( receiveQueueSize == 0 ) {
                palExec_atomicEnd();
                return;
            }

            msg = receiveQueue[receiveQueueHead];
        }
        palExec_atomicEnd();

        msg = mblReceive_receive(msg);

        palExec_atomicBegin();
        {
            receiveQueue[receiveQueueHead] = msg;

            if( ++receiveQueueHead >= RECEIVE_QUEUE_SIZE )
                receiveQueueHead = 0;

            --receiveQueueSize;
        }
        palExec_atomicEnd();
    }
}

//tasklet_async event
message_t* uqlReceive_receive(message_t* msg)
{
    message_t *m;

    palExec_atomicBegin();
    {
        if( receiveQueueSize >= RECEIVE_QUEUE_SIZE ) {
            m = msg;
            pc.numNoBufMbl++;
        } else {
            uint8_t idx = receiveQueueHead + receiveQueueSize;
            if( idx >= RECEIVE_QUEUE_SIZE )
                idx -= RECEIVE_QUEUE_SIZE;

            m = receiveQueue[idx];
            receiveQueue[idx] = msg;

            ++receiveQueueSize;
//            post
            cometos::getScheduler().add(deliverTask);
        }
    }
    palExec_atomicEnd();

    return m;
}
