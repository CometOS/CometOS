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
 * @author Andreas Weigel
 */

#include "mac_constants.h"
#include "tosUtil.h"
#include "cometos.h"
#include "mac_interface.h"
#include "ieee82154.h"
#include "RadioAlarm.h"
#include "RadioConfig.h"
#include "at86rf231_registers.h"
#include <string.h>




/**
 * ugly workaround to convert CometOS mac_interface calls (using data, len, dst),
 * which do only include MAC PAYLOAD,
 * to the clean TinyOS concept of taking the whole message (including any
 * MAC headers) and passing it down the layers of the MAC, with each layer
 * setting/processing the corresponding fields.
 */
bool tosUtil_cometosToMsg(const uint8_t* data, uint8_t len, node_t dst, message_t* msg) {
    ASSERT(msg!= NULL);
    ASSERT(msg->data!=NULL);
    msg->phyFrameLen = len + MAC_HEADER_SIZE;

    msg->requiresCca = true;

    // the FCS is atomatically generated (2 bytes) (TX_AUTO_CRC_ON==1 by default)
    msg->phyPayloadLen = msg->phyFrameLen - 2;

    ASSERT(msg->phyPayloadLen = MAC_HEADER_SIZE -2 + len);
    // frame control lower byte
    if (MAC_BROADCAST == dst) {
        msg->data[0] = 0x41; // no ACK request
    } else {
        msg->data[0] = 0x61; // need ACK TODO could be made depending on settings of mac via setmode
    }

    // frame control upper byte (use 16 bit addressing, IEEE 802.15.4-2003 compliant frame)
    msg->data[1] = 0x88;

    // sequence number is not set, has to be handled by corresponding layer
//    headerBuf[2] = msg->sequence;

    // only intra PAN communication possible with tosMac
    uint16_t networkId = mac_getNetworkId();

    msg->data[3] = networkId & 0xFF;
    msg->data[4] = (networkId >> 8) & 0xFF;

    // write destination
    msg->data[5] = 0xff & dst;
    msg->data[6] = dst >> 8;

    // write source address

    uint16_t nodeId = mac_getNodeId();

    msg->data[7] = nodeId & 0xFF;
    msg->data[8] = (nodeId >> 8) & 0xFF;
    memcpy(&(msg->data[9]), data, len);
    return true;
}


bool tosUtil_rxMsgToCometos(uint8_t* data,
                         uint8_t & len,
                         node_t & dst,
                         node_t & src,
                         mac_networkId_t & srcNwk,
                         mac_networkId_t & dstNwk,
                         mac_phyPacketInfo_t* & rxInfo,
                         message_t* msg) {
    len = msg->phyFrameLen - MAC_HEADER_SIZE;
    dst = tosUtil_getDestAddr(msg);
    src = tosUtil_getSrcAddr(msg);

    dstNwk = tosUtil_getDstPan(msg);
    srcNwk = dstNwk;
    rxInfo = &(msg->rxInfo);
    memcpy(data, &(msg->data[9]), msg->phyPayloadLen);
    return true;
}


bool tosUtil_txMsgToCometos(mac_txInfo_t* & txInfo,
                            message_t* msg) {
    txInfo = &(msg->txInfo);
    return true;
}

bool tosUtil_isAckFrame(message_t* msg) {
    // check type field (bits 0-2) in FCF for 010 pattern,
    return ( (msg->data[0] & IEEE154_TYPE_MASK) == IEEE154_TYPE_ACK );
}

bool tosUtil_isDataFrame(message_t* msg) {
    return ( (msg->data[0] & IEEE154_TYPE_MASK) == IEEE154_TYPE_DATA );
}

void tosUtil_createAckFrame(message_t* msg, message_t* ack) {
    ack->data[0] = IEEE154_TYPE_ACK;
    ack->data[1] = 0x00;  // addressing not present
    ack->data[2] = msg->data[2]; // set sequence number to that of received msg
    msg->requiresCca = false;
}

bool tosUtil_ackRequested(message_t* msg) {
    return msg->data[0] & 0x20;
}void setNetworkId(uint32_t panId);

//uint32_t getNetworkId();

bool tosUtil_requiresAckReply(message_t* msg) {
    return tosUtil_isDataFrame(msg)
            && (msg->data[0] & (1 << IEEE154_FCF_ACK_REQ))
            && (tosUtil_getDestAddr(msg) ==  mac_getNodeId());
}

mac_nodeId_t tosUtil_getDestAddr(message_t* msg) {
    return ( (mac_nodeId_t) msg->data[5] ) | ( ((mac_nodeId_t) msg->data[6]) << 8 );
}


mac_nodeId_t tosUtil_getSrcAddr(message_t* msg) {
    return msg->data[7] | (msg->data[8] << 8);
}

mac_networkId_t tosUtil_getDstPan(message_t* msg) {
    return msg->data[3] | (msg->data[4] << 8);
}

bool tosUtil_isForMe(message_t* msg) {
    mac_nodeId_t dst = tosUtil_getDestAddr(msg);
    return (dst == mac_getNodeId()
                || dst == MAC_BROADCAST)
            && (tosUtil_getDstPan(msg) == mac_getNetworkId());
}

bool tosUtil_wasMsgAcked(message_t* msg) {
    return msg->wasAcked;
}

void tosUtil_setAckReceivedFlag(message_t* msg) {
    if (msg != NULL) {
        msg->wasAcked = true;
    }
}void setNetworkId(uint32_t panId);

uint32_t getNetworkId();

void tosUtil_clearAckReceivedFlag(message_t* msg) {
    if (msg != NULL) {
        msg->wasAcked = false;
    }
}

bool tosUtil_verifyAck(message_t* originalMsg, message_t* ack) {
    return originalMsg->data[2] == ack->data[2]
                && (ack->data[0] & IEEE154_TYPE_MASK) == IEEE154_TYPE_ACK;
}

uint16_t tosUtil_getRetryDelay(message_t* msg) {
    return 0;
}

bool tosUtil_requiresCca(message_t* msg) {
    return msg->requiresCca; // could be replaced by using the setMode interface
}

#define RFA1_BACKOFF_MIN 320

uint16_t tosUtil_getMinBackoff() {
    return (uint16_t)(RFA1_BACKOFF_MIN * RADIO_ALARM_MICROSEC);
}

#define RFA1_BACKOFF_INIT 4960

uint16_t tosUtil_getInitialBackoff() {
    return (uint16_t)(RFA1_BACKOFF_INIT * RADIO_ALARM_MICROSEC);
}

#define RFA1_BACKOFF_CONG 2240

uint16_t tosUtil_getCongestionBackoff() {
    return (uint16_t)(RFA1_BACKOFF_CONG * RADIO_ALARM_MICROSEC);
}

uint16_t tosUtil_getTransmitBarrier(message_t* msg) {
    uint16_t time;

    // TODO: maybe we should use the embedded timestamp of the message
    time = radioAlarm_getNow();

    // estimated response time (download the message, etc) is 5-8 bytes
    if( tosUtil_ackRequested(msg) )
        time += (uint16_t)(32 * (-5 + 16 + 11 + 5) * RADIO_ALARM_MICROSEC);
    else
        time += (uint16_t)(32 * (-5 + 5) * RADIO_ALARM_MICROSEC);

    return time;
}

uint8_t uniqueConfig_getSequenceNumber(message_t* msg) {
    return msg->data[2];
}

void uniqueConfig_setSequenceNumber(message_t* msg, uint8_t seq) {
    msg->data[2] = seq;
}
