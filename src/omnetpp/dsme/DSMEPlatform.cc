/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * described in the IEEE 802.15.4-2015 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
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
#include "openDSME/dsmeLayer/DSMELayer.h"
#include "MacPacket.h"
#include "BaseDecider.h"

using namespace cometos;
using namespace omnetpp;

Define_Module(dsme::DSMEPlatform);

namespace dsme {

void DSMEPlatform::initialize() {
    DSMEPlatformBase::initialize();

    this->dsme.setPHY_PIB(&(this->phy_pib));
    this->dsme.setMAC_PIB(&(this->mac_pib));
    this->dsme.setMCPS(&(this->mcps_sap));
    this->dsme.setMLME(&(this->mlme_sap));

    channelList_t scanChannels;
    scanChannels.add(par("commonChannel"));
    this->dsmeAdaptionLayer.initialize(scanChannels);

    /* Initialize Address */
    IEEE802154MacAddress address;

    uint16_t id = palId_id(); // TODO remove (int)getParentModule()->par("id");
    translateMacAddress(id, this->mac_pib.macExtendedAddress);

    symbolDuration = SimTime(16, SIMTIME_US);
    timer = new Message();
    ccaTimer = new Message();

    this->mac_pib.macShortAddress = this->mac_pib.macExtendedAddress.getShortAddress();

    if(par("isPANCoordinator")) {
        this->mac_pib.macPANId = par("macPANId");
    }

    this->mac_pib.macAssociatedPANCoord = par("isPANCoordinator");
    this->mac_pib.macBeaconOrder = par("beaconOrder");
    this->mac_pib.macSuperframeOrder = par("superframeOrder");
    this->mac_pib.macMultiSuperframeOrder = par("multiSuperframeOrder");

    this->mac_pib.macMinBE = par("macMinBE");
    this->mac_pib.macMaxBE = par("macMaxBE");
    this->mac_pib.macMaxCSMABackoffs = par("macMaxCSMABackoffs");
    this->mac_pib.macMaxFrameRetries = par("macMaxFrameRetries");

    this->mac_pib.macDSMEGTSExpirationTime = par("macDSMEGTSExpirationTime");
    this->mac_pib.macResponseWaitTime = 16;

    this->mac_pib.recalculateDependentProperties();

    this->mac_pib.macIsPANCoord = par("isPANCoordinator");
    this->mac_pib.macIsCoord = (par("isCoordinator") ||  this->mac_pib.macIsPANCoord);

    this->phy_pib.phyCurrentChannel = par("commonChannel");

    if (strcmp(par("allocationScheme").stringValue(), "random") == 0) {
        this->dsmeAdaptionLayer.settings.allocationScheme = DSMEAdaptionLayerSettings::ALLOC_RANDOM;
    } else {
        this->dsmeAdaptionLayer.settings.allocationScheme = DSMEAdaptionLayerSettings::ALLOC_CONTIGUOUS_SLOT;
    }

    this->dsmeAdaptionLayer.setIndicationCallback(DELEGATE(&DSMEPlatform::handleDataMessageFromMCPS, *this));
    this->dsmeAdaptionLayer.setConfirmCallback(DELEGATE(&DSMEPlatform::handleConfirmFromMCPS, *this));

    this->dsme.initialize(this); // should be done after adjusting all the settings
}

void DSMEPlatform::finish() {
    recordScalar("numUpperPacketsForCAP", dsme.getMessageDispatcher().getNumUpperPacketsForCAP());
    recordScalar("numUpperPacketsForGTS", dsme.getMessageDispatcher().getNumUpperPacketsForGTS());
    recordScalar("numUpperPacketsDroppedFullQueue", dsme.getMessageDispatcher().getNumUpperPacketsDroppedFullQueue());
}

bool DSMEPlatform::sendDelayedAck(DSMEMessage *ackMsg, DSMEMessage *receivedMsg, Delegate<void(bool)> txEndCallback) {
    cMessage* acktimer = new cMessage("acktimer");
    acktimer->getParList().setTakeOwnership(false); // ackMsg is still owned by the AckLayer
    acktimer->getParList().addAt(0,ackMsg);

    uint32_t endOfReception = receivedMsg->getStartOfFrameDelimiterSymbolCounter()+receivedMsg->getTotalSymbols()
                                    - 2*4 // Preamble
                                    - 2*1; // SFD
    uint32_t ackTime = endOfReception + aTurnaroundTimeSymbols;
    uint32_t now = getSymbolCounter();
    uint32_t diff = ackTime-now;

    scheduleAt(simTime() + diff*symbolDuration, acktimer);
    return true;
}


void DSMEPlatform::receiveLowerData(cMessage *msg) {
    // Get data from Mac Packet
    checked_ptr<MacPacket> pktMac(check_and_cast<MacPacket*>(msg));
    AirframePtr pkt = pktMac->decapsulateNewAirframe();
    pktMac.delete_object();

    // Remove the FCS (only dummy bytes since the actual bit error calculation is not based on the FCS itself)
    uint16_t fcs;
    (*pkt) >> fcs;

    DSMEMessage* dsmemsg = getLoadedMessage(pkt);
    pkt.reset();

    dsmemsg->getHeader().decapsulateFrom(dsmemsg);

    dsmemsg->startOfFrameDelimiterSymbolCounter = getSymbolCounter() - dsmemsg->getTotalSymbols()
                            + 2*4 // Preamble
                            + 2*1; // SFD

    dsme.getAckLayer().receive(dsmemsg);
}

void DSMEPlatform::handleMessage(cMessage* msg) {
    if(msg == timer) {
        dsme.getEventDispatcher().timerInterrupt();
    }
    else if(msg == ccaTimer) {
        bool isIdle = !isChannelBusy();
        LOG_DEBUG("CCA isIdle " << isIdle);
        dsme.dispatchCCAResult(isIdle);
    }
    else if(strcmp(msg->getName(),"acktimer") == 0) {
        //LOG_INFO("send ACK")
        bool result = prepareSendingCopy((DSMEMessage*)msg->getParList().get(0), txEndCallback);
        ASSERT(result);
        result = sendNow();
        ASSERT(result);
        // the ACK Message itself will be deleted inside the AckLayer
        delete msg;
    }
    else {
        MacAbstractionLayer::handleMessage(msg);
    }
}

void DSMEPlatform::txDone(macTxResult_t result) {
    LOG_DEBUG("Sig txDone for pckt with ID");

    ASSERT(txPkt);

    MacTxInfo info(0, 0);

    txPkt.delete_object();

    if (result == MTR_SUCCESS)
        sendingSucceed++;
    else
        sendingFailed++;

    txEnd(result, info);
}

void DSMEPlatform::receiveLowerControl(cMessage *msg) {

    LOG_DEBUG("receive ctrl "<<msg->getKind() );

    if (msg->getKind() == MacToPhyInterface::TX_OVER) {
        LOG_DEBUG("SetRadioState: " << Radio::RX);
        phy->setRadioState(Radio::RX);
        MacEvent e(MacEvent::FRAME_TRANSMITTED);
        dispatch(e);
    } else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
        //ASSERT(phy->getRadioState() == Radio::RX);

        // we additionally dispatch a frame dropped event to handle a situation
        // in which we waited with TX for RX of a frame which then failed --
        // we only dispatch the event if the frame was dropped due to a bit
        // error, i.e. simulated CRC failure and NOT due to a missed SFD
        // --- this is also the logic we apply when we decide to dispatch
        // a send request in the first place, i.e. we wait if currently a
        // frame is received
        if (phy->getChannelState().isIdle()) {
            MacEvent e(MacEvent::EXPECTED_FRAME_DROPPED);
            dispatch(e);
        }
    } else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
        MacEvent e(MacEvent::RADIO_SWITCHING_OVER);
        dispatch(e);
    } else {
        LOG_ERROR("MsgKind " << msg->getKind());
        ASSERT(false);
    }
    delete msg;
}

void DSMEPlatform::signalNewMsg(DSMEMessage* msg) {
#if 0
    LOG_INFO_PREFIX;
    LOG_INFO_PURE("signalNewMsg " << msgId << " - ");
    for(auto i : msgsActive) {
        LOG_INFO_PURE(i << " ");
    }
    LOG_INFO_PURE(cometos::endl);
#endif

    msgMap[msg] = msgId;
    msgsActive.insert(msgId);
    msgId++;
}

void DSMEPlatform::signalReleasedMsg(DSMEMessage* msg) {
    auto id = msgMap[msg];
    auto it = msgsActive.find(id);
    DSME_ASSERT(it != msgsActive.end());
    msgsActive.erase(it);

#if 0
    LOG_INFO_PREFIX;
    LOG_INFO_PURE("signalReleasedMsg " << id << " - ");
    for(auto i : msgsActive) {
        LOG_INFO_PURE(i << " ");
    }
    LOG_INFO_PURE(cometos::endl);
#endif
}

bool DSMEPlatform::prepareSendingCopy(DSMEMessage* msg, Delegate<void(bool)> txEndCallback) {
    printSequenceChartInfo(msg);

    if (msg == nullptr) {
        return false;
    }

    LOG_INFO("sendCopyNow " << (uint64_t)msg);

    this->txEndCallback = txEndCallback;
    AirframePtr frame = msg->getSendableCopy();

    // Add the FCS (only dummy bytes since the actual bit error calculation is not based on the FCS itself)
    uint16_t fcs = 0;
    (*frame) << fcs;

    currTxPower = txPower;

    if (txPkt) {
        frame.delete_object();
        return false;
    }

    if (enable == false) {
        frame.delete_object();
        return false;
    }

    // store  and prepare packet for transmission
    txPkt = frame;
    txMode = 0;

    // resetting parameters for transmission
    resetTxParams();

    bool isReceiving = !phy->getChannelState().isIdle();
    startSendingImmediately = false;

    if (!txWaitsForBusyRx) {
        startSendingImmediately = true;
    }

    // check if we are currently receiving anything
    if (isReceiving) {
        // the phy channel is currently trying to decode an incoming frame,
        // record this
        txDuringRx++;
    } else {
        // if the channel is idle, sending start in any case
        startSendingImmediately = true;
    }

    return true;
}

bool DSMEPlatform::sendNow() {
    if (startSendingImmediately) {
        startSendingImmediately = false;
        LOG_DEBUG("init sending process");
        MacEvent e(MacEvent::SEND_REQUEST);
        dispatch(e);
    }
    return true;
}

void DSMEPlatform::abortPreparedTransmission() {
    startSendingImmediately = false;
}

bool DSMEPlatform::setChannelNumber(uint8_t k) {
    DSME_ASSERT(k >= 11 && k <= 26);
    phy->setCurrentRadioChannel(k);
    return true;
}

}
