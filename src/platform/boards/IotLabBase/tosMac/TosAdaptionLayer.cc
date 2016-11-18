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

#include "TosAdaptionLayer.h"
#include "tosUtil.h"
#include "UniqueLayer.h"
#include "mac_interface.h"
#include "MessageBufferLayer.h"
#include "SoftwareAckLayer.h"
#include "RFA1DriverLayer.h"
#include "palExec.h"
#include "cometos.h"
#include "RadioAlarm.h"
#include "pinEventOutput.h"
#include "PhyCount.h"

extern cometos::PhyCounts pc;

static uint8_t* frameBuf = NULL;
static const uint8_t* txBuf = NULL;
static bool promiscuousModeEnabled = false;

mac_result_t mac_send(uint8_t const* data,
                      mac_payloadSize_t length,
                      mac_nodeId_t dst) {
    static uint8_t dataBuf[MAC_MAX_PAYLOAD_SIZE + MAC_HEADER_SIZE];
    static TosMsg msg;
    msg.data = dataBuf;

    txBuf = data;
    bool success = tosUtil_cometosToMsg(data, length, dst, &msg);
    if (success) {
        EVENT_OUTPUT_WRITE(PEO_TX_START);
        return uqlSend_send(&msg);
    } else {
        return MAC_ERROR_BUSY;
    }
}


mac_result_t mac_sendToNetwork(uint8_t const* data,
                      mac_payloadSize_t length,
                      mac_nodeId_t dst,
                      mac_networkId_t nwk) {
    // TODO implement
    return MAC_ERROR_FAIL;
}

void combine(mac_result_t & result, mac_result_t other) {
    result = (result == other ? result : MAC_ERROR_FAIL);
}

mac_result_t mac_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
        mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
        mac_backoffCfg_t *backoffCfg) {
    mac_result_t result;
    mac_result_t tmp;

    radioAlarm_init();
    result = RFA1Driver_init(myAddr, nwkId, channel, mode, ackCfg, backoffCfg);

    tmp = mbl_init();

    combine(result, tmp);

    tmp = sal_init();

    combine(result, tmp);

    tmp = mac_setBackoffConfig(backoffCfg);

    combine(result, tmp);

    tmp = mac_setAutoAckConfig(ackCfg);

    combine(result, tmp);

    return result;
}

mac_result_t mac_setReceiveBuffer(uint8_t* buf) {
    if (frameBuf != NULL) {
        return MAC_ERROR_ALREADY;
    } else {
        frameBuf = buf;
        return MAC_SUCCESS;
    }
}


void mac_setPromiscuousMode(bool value) {
    promiscuousModeEnabled = value;
}

bool mac_getPromiscuousMode() {
    return promiscuousModeEnabled;
}

message_t* mblReceive_receive(message_t* msg) {
    uint8_t len;
    node_t src, dst;
    mac_networkId_t srcNwk, dstNwk;
    mac_phyPacketInfo_t * rxInfo;


    bool bufAvailable;
    palExec_atomicBegin(); // not strictly necessary, receive called in task context
    bufAvailable = frameBuf != NULL;
    palExec_atomicEnd();
    if (bufAvailable) {
        if (tosUtil_isForMe(msg) || promiscuousModeEnabled) {
            EVENT_OUTPUT_WRITE(PEO_RX_DONE);
            uint8_t* originalRxBuf = frameBuf;
            bool result = tosUtil_rxMsgToCometos(frameBuf, len, dst, src, srcNwk, dstNwk, rxInfo, msg);
            ASSERT(result);
            frameBuf = NULL;
            mac_cbReceive(originalRxBuf, len, dst, src, srcNwk, dstNwk, rxInfo);
        } else {
            EVENT_OUTPUT_WRITE(PEO_RX_DROPPED);
        }
    } else {
        pc.numNoBufTosAdapt++;
    }
    return msg;
}

void uqlSend_sendDone(message_t * msg, mac_result_t error) {
    mac_txInfo_t * txInfo;
    const uint8_t* originalDataBuf = txBuf;
    txBuf = NULL;
    tosUtil_txMsgToCometos(txInfo, msg);

    mac_cbSendDone(originalDataBuf, error, txInfo);
}


