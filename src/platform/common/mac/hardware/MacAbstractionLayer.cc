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

#include "MacAbstractionLayer.h"
#include "MacControl.h"
#include "MacAddressingBase.h"
#include "ParameterStore.h"
#include "OverwriteAddrData.h"

#include "palLed.h"
#include "palId.h"
#include "palLocalTime.h"

#if !(defined AIRFRAME_MAX_SIZE) || !(defined MAC_PACKET_BUFFER_SIZE) || AIRFRAME_MAX_SIZE != MAC_PACKET_BUFFER_SIZE
#error "AIRFRAME_MAX_SIZE and MAC_PACKET_BUFFER_SIZE have to be equal"
#endif

#include "OutputStream.h"

#if defined BOARD_M3OpenNode or defined BOARD_A8_M3
extern cometos::PhyCounts pc;
#endif

namespace cometos {

MacAbstractionLayer * MacAbstractionLayer::mal = NULL;

PhyCounts MacAbstractionLayer::getPhyStats() {
#if defined BOARD_M3OpenNode or defined BOARD_A8_M3
    return pc;
#endif
    return PhyCounts();
}

void MacAbstractionLayer::resetPhyStats() {
#if defined BOARD_M3OpenNode or defined BOARD_A8_M3
    pc = PhyCounts();
#endif
}

MacAbstractionLayer::MacAbstractionLayer(const char* name,
                                         const node_t* fixedAddress) :
		MacAbstractionBase(name),
		gateIndOut(this, "gateIndOut"),
        gateSnoopIndOut(this, "gateSnoopIndOut"),
        rxMsg(NULL),
        txMsg(NULL),
        fixedAddress(fixedAddress),
        changeDriverConfigPending(false)
{
}

mac_result_t MacAbstractionLayer::changeDriverConfig(const MacConfig & cfg) {
    mac_ackCfg_t ackCfg;
    ackCfg.maxFrameRetries = cfg.maxFrameRetries;
    ackCfg.ackWaitDuration = cfg.ackWaitDuration;

    mac_backoffCfg_t boCfg;
    boCfg.minBE = cfg.minBE;
    boCfg.maxBE = cfg.maxBE;
    boCfg.maxBackoffRetries = cfg.maxBackoffRetries;
    boCfg.unitBackoff = cfg.unitBackoff;

    mac_result_t result = MAC_SUCCESS;
    mac_setChannel(cfg.channel);
    ASSERT(result == MAC_SUCCESS);
    result = mac_setAutoAckConfig(&ackCfg);
    ASSERT(result == MAC_SUCCESS);
    result = mac_setBackoffConfig(&boCfg);
    ASSERT(result == MAC_SUCCESS);
    mac_setCCAMode(cfg.ccaMode);
    ASSERT(result == MAC_SUCCESS);
    mac_setCCAThreshold(cfg.ccaThreshold);
    ASSERT(result == MAC_SUCCESS);
    mac_setNetworkId(cfg.nwkId);
    ASSERT(result == MAC_SUCCESS);
    mac_setMode(cfg.macMode);
    ASSERT(result == MAC_SUCCESS);
    setPromiscuousMode(cfg.promiscuousMode);
    return result;
}

void MacAbstractionLayer::initialize() {
	MacAbstractionBase::initialize();
	
	remoteDeclare(&MacAbstractionLayer::getStandardConfig, "gsc");
	remoteDeclare(&MacAbstractionLayer::setStandardConfig, "ssc");
	remoteDeclare(&MacAbstractionLayer::resetStandardConfig, "rsc");
	remoteDeclare(&MacAbstractionLayer::getActiveConfig, "gac");
#if defined BOARD_M3OpenNode or defined BOARD_A8_M3
    remoteDeclare(&MacAbstractionLayer::getPhyStats, "gphy");
    remoteDeclare(&MacAbstractionLayer::resetPhyStats, "rphy");
#endif
	// if there is a ParameterStore, try to load configuration from there
	ParameterStore * ps = (ParameterStore *) getModule(ParameterStore::MODULE_NAME);
	if (ps != NULL) {
	    cometos_error_t result;
	    MacConfig tmpcfg;
	    result = ps->getCfgData(this, tmpcfg);
	    if (result == COMETOS_SUCCESS){
	        standardConfig = tmpcfg;
	    }
	}

	mac_txMode_t mode = standardConfig.macMode;

	ASSERT(mal == NULL);
	mal = this;

	mac_ackCfg_t ackCfg;
	mac_backoffCfg_t boCfg;
	ackCfg.maxFrameRetries = standardConfig.maxFrameRetries;
	ackCfg.ackWaitDuration = standardConfig.ackWaitDuration;
	boCfg.minBE = standardConfig.minBE;
	boCfg.maxBE = standardConfig.maxBE;
	boCfg.maxBackoffRetries = standardConfig.maxBackoffRetries;
	boCfg.unitBackoff = standardConfig.unitBackoff;

	if (fixedAddress != NULL) {
	    address = *fixedAddress;
	} else {
#ifdef PAL_ID
	    address = palId_id();
    #else
	    address = MAC_BROADCAST;
#endif
	}

	mac_result_t result = mac_init(address,
                                   standardConfig.nwkId,
                                   standardConfig.channel,
							       mode,
							       &ackCfg,
							       &boCfg);
	ASSERT(result == MAC_SUCCESS);
	result = mac_setCCAThreshold(standardConfig.ccaThreshold);
	ASSERT(result == MAC_SUCCESS);
	result = mac_setCCAMode(standardConfig.ccaMode);
	ASSERT(result == MAC_SUCCESS);
	rxMsg = new Airframe();
	result = mac_setReceiveBuffer(rxMsg->getData());
	result = mac_on();
	ASSERT(result == MAC_SUCCESS);
	setPromiscuousMode(standardConfig.promiscuousMode);
}


void MacAbstractionLayer::finish() {

}

cometos_error_t MacAbstractionLayer::resetStandardConfig() {
    ParameterStore * ps = (ParameterStore *) getModule(ParameterStore::MODULE_NAME);
    cometos_error_t result = COMETOS_SUCCESS;
    if (ps != NULL) {
        result = ps->resetCfgData(this);
    }
    return result;
}

MacConfig  MacAbstractionLayer::getStandardConfig() {
    ParameterStore * ps = (ParameterStore *) getModule(ParameterStore::MODULE_NAME);
    MacConfig tmpcfg = standardConfig;
    if (ps != NULL) {
        ps->getCfgData(this, tmpcfg);
    } else {
        tmpcfg.setPersistent(false);
    }

    tmpcfg.setEqualToActiveConfig(standardConfig == getActiveConfig());

    return tmpcfg;
}

MacConfig MacAbstractionLayer::getActiveConfig() {
    MacConfig cfg;
    mac_ackCfg_t ackCfg;
    mac_backoffCfg_t backoffCfg;

    mac_getAutoAckConfig(&ackCfg);
    mac_getBackoffConfig(&backoffCfg);

    cfg.ackWaitDuration = ackCfg.ackWaitDuration;
    cfg.maxFrameRetries = ackCfg.maxFrameRetries;

    cfg.maxBE = backoffCfg.maxBE;
    cfg.minBE = backoffCfg.minBE;
    cfg.maxBackoffRetries = backoffCfg.maxBackoffRetries;
    cfg.unitBackoff = backoffCfg.unitBackoff;

    cfg.ccaMode = mac_getCCAMode();
    cfg.ccaThreshold = mac_getCCAThreshold();
    cfg.channel = mac_getChannel();
    cfg.macMode = mac_getMode();
    cfg.nwkId = mac_getNetworkId();
    cfg.txPower = standardConfig.txPower;
    cfg.promiscuousMode = mac_getPromiscuousMode();
    cfg.setEqualToActiveConfig(true);

    return cfg;
}

cometos_error_t MacAbstractionLayer::setStandardConfig(MacConfig & cfg) {
    if (cfg.isValid()) {
        standardConfig = cfg;
        ParameterStore * ps = (ParameterStore *) getModule(ParameterStore::MODULE_NAME);
        if (ps != NULL) {
            ps->setCfgData(this, standardConfig);
        }

        if (txMsg != NULL) {
            // we defer changing the driver config, because we do not know in which
            // state the mac driver currently is -- we do this next time before sending
            changeDriverConfigPending = true;
        } else {
            changeDriverConfig(standardConfig);
        }
        return COMETOS_SUCCESS;
    } else {
        return COMETOS_ERROR_INVALID;
    }
}

bool MacAbstractionLayer::sleep() {
	mac_result_t result = mac_sleep();
	if (result == MAC_SUCCESS || result == MAC_ERROR_ALREADY) {
		return true;
	} else {
		return false;
	}
}

bool MacAbstractionLayer::listen() {
	mac_result_t result = mac_on();
	if (result == MAC_SUCCESS || result == MAC_ERROR_ALREADY) {
		return true;
	} else {
		return false;
	}
}

bool MacAbstractionLayer::sendAirframe(Airframe* frame, node_t dst, uint8_t mode, const ObjectContainer* meta) {
	ASSERT(txMsg == NULL);
	if (dst == mac_getNodeId()) {
        delete frame;
        LOG_WARN("frame discarded; destination==nodeId");
        return false;
    }

	txMsg = frame;
	mti.destination = dst;

	mac_result_t result = MAC_ERROR_FAIL;

	if (changeDriverConfigPending) {
	    changeDriverConfigPending = false;
	    changeDriverConfig(standardConfig);
	}

	result = mac_setMode(mode);


	if (result == MAC_SUCCESS) {
	    mac_power_t pwrLvl = standardConfig.txPower;
		if (meta->has<MacControl>()) {
            MacControl* mc = meta->get<MacControl>();
            mac_power_t maxLvl = mac_getMaxTxPowerLvl();
            mac_power_t minLvl = mac_getMinTxPowerLvl();
            uint16_t tmpPwr = ((uint16_t) mc->txPower) * (maxLvl - minLvl);
            pwrLvl = (tmpPwr / 255) + minLvl;
        }

		result = mac_setTxPower(pwrLvl);
	}

	if (result == MAC_SUCCESS) {
	    node_t srcAddr = address;
	    if (meta->has<OverwriteAddrData>()) {
	        OverwriteAddrData* oad = meta->get<OverwriteAddrData>();
	        srcAddr = oad->src;
	    }
	    result = mac_setNodeId(srcAddr);
	}


	if (result == MAC_SUCCESS) {
	    mac_ackCfg_t ackCfg;
	    ackCfg.maxFrameRetries = MAC_DEFAULT_FRAME_RETRIES;
	    ackCfg.ackWaitDuration = MAC_DEFAULT_ACK_WAIT_DURATION;

	    if (meta->has<MacMetaRetries>()) {
	        MacMetaRetries * mmr = meta->get<MacMetaRetries>();
            ackCfg.maxFrameRetries = mmr->_retries;
	    }

        result = mac_setAutoAckConfig(&ackCfg);
	}

    if (result == MAC_SUCCESS) {
        sendTime = palLocalTime_get();
        if (meta->has<DestinationNwk>()) {
            LOG_DEBUG("SEND TO NETWORK nwkId=" << (int) frame->get<DestinationNwk>()->dstNwk);
            mac_networkId_t dstNwk = frame->get<DestinationNwk>()->dstNwk;
            ASSERT(false);
            result = mac_sendToNetwork(frame->getData(),
                                       frame->getLength(),
                                       dst,
                                       dstNwk);
        } else {
            LOG_DEBUG("SEND");
            LOG_ERROR("S:" << dst);
            result = mac_send(frame->getData(),
                              frame->getLength(),
                              dst);
        }
    }

	if (result != MAC_SUCCESS) {
	    delete(txMsg);
		txMsg = NULL;
		return false;
	} else {
		return true;
	}
}


bool MacAbstractionLayer::setNwkId(mac_networkId_t nwkId) {
	mac_setNetworkId(nwkId);
	return true;
}

bool MacAbstractionLayer::setShortAddr(node_t newAddr) {
	return mac_setNodeId(newAddr);
}

node_t MacAbstractionLayer::getShortAddr() {
	return mac_getNodeId();
}

void MacAbstractionLayer::rxEnd(Airframe *frame, node_t src, node_t dst, MacRxInfo const & info) {
	ASSERT(false);
}

void MacAbstractionLayer::txEnd(macTxResult_t result, MacTxInfo const & info) {
	ASSERT(false);
}

void MacAbstractionLayer::rxDropped() {
	//printf("DROPPED------\n");
}

bool MacAbstractionLayer::configureBackoffAlgorithm(uint8_t minBE,
		uint8_t maxBE, uint8_t maxBackoffRetries) {
	mac_backoffCfg_t boCfg;
	boCfg.minBE = minBE;
	boCfg.maxBE = maxBE;
	boCfg.maxBackoffRetries = maxBackoffRetries;
	mac_result_t result = mac_setBackoffConfig(&boCfg);

	if (result != MAC_SUCCESS) {
		return false;
	} else {
		return true;
	}
}

void MacAbstractionLayer::setPromiscuousMode(bool value) {
	mac_setPromiscuousMode(value);
	ASSERT(mac_getPromiscuousMode() == value);
}

void MacAbstractionLayer::processRxDropped(Message *msg) {
	rxDropped();
}

void MacAbstractionLayer::processTxDone(Message *msg) {
	delete(txMsg);
	txMsg = NULL;
	//printf("macTxDn: res=%d|res=%d|dst=%d|rtr=%d|rRs=%d|aRs=%d\n", result, txResult, mti.destination, mti.numRetries, mti.remoteRssi, mti.ackRssi);
	txEnd(txResult, mti);
}

void MacAbstractionLayer::processRxDone(Message * msg) {
	Airframe * justReceived = rxMsg;

	// pass current message to actual mac layer for logging, stats etc.
	rxEnd(justReceived, rxSrc, rxDst, ppi);
	// send up indication
    DataIndication * ind = new DataIndication(justReceived, rxSrc, rxDst);
    ind->set((MacRxInfo*) ppi.getCopy());
    LOG_ERROR("R s=" << rxSrc << " d=" << rxDst);
    if (ind->dst == mac_getNodeId() || ind->dst == MAC_BROADCAST) {
        gateIndOut.send(ind);
    } else {
        gateSnoopIndOut.send(ind);
    }

	// create new receive buffer and pass to mac layer
	rxMsg = new Airframe();
	mac_setReceiveBuffer(rxMsg->getData());
}

void MacAbstractionLayer::rxDroppedCallback() {
	schedule(&rxDroppedEvent, &MacAbstractionLayer::processRxDropped);
}

void MacAbstractionLayer::rxCallback(uint8_t * buffer,
									 uint8_t len,
									  mac_phyPacketInfo_t const * phyInfo,
									  node_t dst,
									  node_t src,
									  mac_networkId_t nwkId) {
	// received data contained in Airframe of rxMsg
	ASSERT(rxMsg->getData() == buffer);
	rxMsg->setLength(len);
	ASSERT(rxMsg->getLength() == len);
	rxDst = dst;
	rxSrc = src;
	LOG_ERROR("R:n=" << (int)nwkId <<"|myN="<< (int)mac_getNetworkId() << "|d=" << rxDst << "|s=" << rxSrc);
	ASSERT(phyInfo != NULL);
	ppi.lqiIsValid = phyInfo->lqiIsValid;
	ppi.lqi = phyInfo->lqi;
	ppi.rssi = phyInfo->rssi;
	ppi.tsInfo.isValid = phyInfo->tsData.isValid;
	ppi.tsInfo.ts = phyInfo->tsData.ts;
	schedule(&rxEvent, &MacAbstractionLayer::processRxDone);
}

void MacAbstractionLayer::txDoneCallback(uint8_t const * data,
										 mac_result_t result,
		                                 mac_txInfo_t const * macInfo) {
	// possible overflow does not matter for difference
    mti.txDuration = palLocalTime_get() - sendTime;
    mti.remoteRssi = MAC_RSSI_INVALID;
    mti.ackRssi = MAC_RSSI_INVALID;
    ASSERT(data == txMsg->getData());
    switch(result) {
        case MAC_SUCCESS: {
            txResult = MTR_SUCCESS;
            if (mti.destination != MAC_BROADCAST) {
                mti.remoteRssi = macInfo->remoteRssi;
                mti.ackRssi = macInfo->ackRssi;
            }
            break;
        }
        case MAC_ERROR_BUSY: {
            txResult = MTR_CHANNEL_ACCESS_FAIL;
            break;
        }
        case MAC_ERROR_NO_ACK: {
            txResult = MTR_NO_ACK;
            break;
        }
        case MAC_ERROR_INVALID: {
            txResult = MTR_INVALID;
            break;
        }
        default: {
            txResult = MTR_INVALID;
            break;
        }
    }

	mti.tsInfo.ts = macInfo->tsData.ts;
	mti.tsInfo.isValid = macInfo->tsData.isValid;
	this->result = result;
	mti.numRetries = macInfo->numRetransmissions;
	mti.numCCARetries = macInfo->numBackoffs;
	schedule(&txEvent, &MacAbstractionLayer::processTxDone);
}


} // namespace cometos

/* ***************************************************************************
 *  Implementation of callback functions *************************************
 ****************************************************************************/
extern "C" {
void mac_cbReceive(uint8_t * buffer, mac_payloadSize_t length, mac_nodeId_t dst,
		mac_nodeId_t src, mac_networkId_t srcNwkId, mac_networkId_t dstNwkId,
		mac_phyPacketInfo_t const * info) {
	if (cometos::MacAbstractionLayer::mal != NULL) {
		cometos::MacAbstractionLayer::mal->rxCallback(buffer, length, info, dst,
				src, dstNwkId);
	}
}

void mac_cbDropped() {
	if (cometos::MacAbstractionLayer::mal != NULL) {
		cometos::MacAbstractionLayer::mal->rxDroppedCallback();
	}
}

void mac_cbSendDone(const uint8_t * data, mac_result_t status,
		mac_txInfo_t const * info) {
	if (cometos::MacAbstractionLayer::mal != NULL) {
		cometos::MacAbstractionLayer::mal->txDoneCallback(data, status, info);
	}
}

} // extern "C"
