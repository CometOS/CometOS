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
#include "logging.h"
#include "RadioAlarm.h"

static uint8_t* frameBuf = NULL;
static const uint8_t* txBuf = NULL;
static bool promiscuousModeEnabled = false;

mac_backoffCfg_t csmaCfg;

mac_result_t mac_setBackoffConfig(const mac_backoffCfg_t *cfg) {
    csmaCfg.maxBE = cfg->maxBE;
    csmaCfg.minBE = cfg->minBE;
    csmaCfg.maxBackoffRetries = cfg->maxBackoffRetries;
    csmaCfg.unitBackoff = cfg->unitBackoff;
    return MAC_SUCCESS;
}

void mac_getBackoffConfig(mac_backoffCfg_t * cfg) {
    cfg->maxBE = csmaCfg.maxBE;
    cfg->minBE = csmaCfg.minBE;
    cfg->maxBackoffRetries = csmaCfg.maxBackoffRetries;
    cfg->unitBackoff = csmaCfg.unitBackoff;
};


mac_result_t mac_setAutoAckConfig(const mac_ackCfg_t *cfg) {
    return MAC_SUCCESS;
}

void mac_getAutoAckConfig(mac_ackCfg_t* cfg) {
}

mac_result_t mac_send(uint8_t const* data,
                      mac_payloadSize_t length,
                      mac_nodeId_t dst) {
    static uint8_t dataBuf[MAC_MAX_PAYLOAD_SIZE + MAC_HEADER_SIZE];
    static message_t msg;
    msg.data = dataBuf;

    txBuf = data;
    bool success = tosUtil_cometosToMsg(data, length, dst, &msg);
    if (success) {
        return radioSend_send(&msg);
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

    result = RFA1Driver_init(myAddr, nwkId, channel, mode, ackCfg, backoffCfg);

    return result;
}

mac_result_t mac_setReceiveBuffer(uint8_t* buf) {
    palExec_atomicBegin();
    if (frameBuf != NULL) {
        palExec_atomicEnd();
        return MAC_ERROR_ALREADY;
    } else {
        frameBuf = buf;
        palExec_atomicEnd();
        return MAC_SUCCESS;
    }
}


void mac_setPromiscuousMode(bool value) {
    promiscuousModeEnabled = value;
}

bool mac_getPromiscuousMode() {
    return promiscuousModeEnabled;
}

message_t* radioReceive_receive(message_t* msg) {
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
            uint8_t* originalRxBuf = frameBuf;
            bool result = tosUtil_rxMsgToCometos(frameBuf, len, dst, src, srcNwk, dstNwk, rxInfo, msg);
            ASSERT(result);
            frameBuf = NULL;
            mac_cbReceive(originalRxBuf, len, dst, src, srcNwk, dstNwk, rxInfo);
        }
    }
    return msg;
}

void radioSend_sendDone(message_t * msg, mac_result_t error) {
    mac_txInfo_t * txInfo;
    const uint8_t* originalDataBuf = txBuf;
    txBuf = NULL;
    tosUtil_txMsgToCometos(txInfo, msg);

    mac_cbSendDone(originalDataBuf, error, txInfo);
}


