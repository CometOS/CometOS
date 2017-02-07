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

#include "DSMEPlatformBase.h"
#include "openDSME/dsmeLayer/messages/MACCommand.h"
#include "openDSME/dsmeLayer/DSMELayer.h"

#include "palId.h"
#include "logging.h"

using namespace cometos;

namespace dsme {

void DSMEPlatformBase::translateMacAddress(node_t& from, IEEE802154MacAddress& to) {
    if(from == 0xFFFF) {
        to = IEEE802154MacAddress(IEEE802154MacAddress::SHORT_BROADCAST_ADDRESS);
    } else {
        to.setShortAddress(from);
    }
}

DSMEPlatformBase::DSMEPlatformBase(const char* service_name) :
                cometos::MacAbstractionLayer(service_name),
                gateReqIn(this, &DSMEPlatformBase::handleRequest, "gateReqIn"),
#ifdef USE_ONE_CHANNEL_ONLY
                phy_pib(10,true),
#else
                phy_pib(10),
#endif
                mac_pib(phy_pib),

                mcps_sap(dsme),
                mlme_sap(dsme),
                dsmeAdaptionLayer(dsme),

                messagesInUse(0),

                startTask(CALLBACK_MET(&DSMEPlatformBase::start,*this)),
                handleStartOfCFPTask(CALLBACK_MET(&DSMELayer::handleStartOfCFP,dsme))
{
}

DSMEPlatformBase::~DSMEPlatformBase() {
}

void DSMEPlatformBase::initialize() {
    MacAbstractionLayer::initialize();
    cometos::getScheduler().add(startTask);
}

void DSMEPlatformBase::start() {
    dsme.start();
    this->dsmeAdaptionLayer.startAssociation();
}

void DSMEPlatformBase::handleDataMessageFromMCPS(IDSMEMessage* imsg) {
    DSMEMessage* msg = static_cast<DSMEMessage*>(imsg);
    AirframePtr macPkt = msg->decapsulateFrame();
    cometos::DataIndication* ind = new cometos::DataIndication(macPkt, msg->getHeader().getSrcAddr().getShortAddress(), msg->getHeader().getDestAddr().getShortAddress());
    ind->set(new cometos::MacRxInfo(msg->rxInfo));
    releaseMessage(msg);
    this->gateIndOut.send(ind);
}

void DSMEPlatformBase::handleConfirmFromMCPS(IDSMEMessage* imsg, DataStatus::Data_Status dataStatus) {
    DSMEMessage* msg = static_cast<DSMEMessage*>(imsg);
    if(msg->request != nullptr) {
        DataResponseStatus status = DataResponseStatus::FAIL_UNKNOWN;
        switch(dataStatus) {
        case DataStatus::SUCCESS:
            status = DataResponseStatus::SUCCESS;
            break;
        case DataStatus::INVALID_GTS:
            status = DataResponseStatus::INVALID_GTS;
            break;
        case DataStatus::NO_ACK:
            status = DataResponseStatus::NO_ACK;
            break;
        case DataStatus::TRANSACTION_OVERFLOW:
            status = DataResponseStatus::QUEUE_FULL;
            break;
        case DataStatus::TRANSACTION_EXPIRED:
            status = DataResponseStatus::EXPIRED;
            break;
        case DataStatus::CHANNEL_ACCESS_FAILURE:
            status = DataResponseStatus::CHANNEL_ACCESS_FAILURE;
            break;
        default:
            ASSERT(false);
        }

        msg->request->response(new cometos::DataResponse(status));
    }

    releaseMessage(msg);
}

bool DSMEPlatformBase::isReceptionFromAckLayerPossible() {
    return (handleMessageTask.isOccupied() == false);
}

void DSMEPlatformBase::handleReceivedMessageFromAckLayer(IDSMEMessage* message) {
    ASSERT(handleMessageTask.isOccupied() == false);
    bool success = handleMessageTask.occupy(message, this->receiveFromAckLayerDelegate);
    ASSERT(success); // assume that isMessageReceptionFromAckLayerPossible was called before in the same atomic block
    cometos::getScheduler().add(handleMessageTask);
}

void DSMEPlatformBase::scheduleStartOfCFP() {
    cometos::getScheduler().replace(handleStartOfCFPTask);
}

void DSMEPlatformBase::setReceiveDelegate(receive_delegate_t receiveDelegate) {
    this->receiveFromAckLayerDelegate = receiveDelegate;
}

void DSMEPlatformBase::handleRequest(DataRequest* req) {
    LOG_INFO_PREFIX;
    LOG_INFO_PURE("Upper layer requests to send a message to ");

    DSMEMessage* dsmemsg = getLoadedMessage(req->decapsulateAirframe());
    dsmemsg->request = req;

    auto& hdr = dsmemsg->getHeader();
    hdr.setFrameType(IEEE802154eMACHeader::DATA);
    hdr.setSrcAddr(this->mac_pib.macExtendedAddress);
    node_t dest = req->dst;
    LOG_INFO_PURE(dest);
    LOG_INFO_PURE("." << cometos::endl);

    translateMacAddress(dest, dsmemsg->getHeader().getDestAddr());

    this->dsmeAdaptionLayer.sendMessage(dsmemsg);
}

void DSMEPlatformBase::txEnd(macTxResult_t result, MacTxInfo const & info) {
    LOG_DEBUG("txEnd");

    if (txEndCallback) {
        txEndCallback(result == macTxResult_t::MTR_SUCCESS);
    }
}

void DSMEPlatformBase::rxEnd(AirframePtr frame, node_t src, node_t dst, MacRxInfo const & info) {
    // sending the indication is handled by receiveLowerData
}

void DSMEPlatformBase::printDSMEManagement(uint8_t management, DSMESABSpecification& subBlock, CommandFrameIdentifier cmd) {
    uint8_t numChannels = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumChannels();
    uint8_t numGTSlots = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumGTSlots();
    uint8_t numSuperFramesPerMultiSuperframe = this->dsmeAdaptionLayer.getMAC_PIB().helper.getNumberSuperframesPerMultiSuperframe();

    LOG_INFO_PURE(" ");
    uint8_t type = management & 0x7;
    switch ((ManagementType) type) {
    case DEALLOCATION:
        LOG_INFO_PURE("DEALLOCATION");
        break;
    case ALLOCATION:
        LOG_INFO_PURE("ALLOCATION");
        break;
    case DUPLICATED_ALLOCATION_NOTIFICATION:
        LOG_INFO_PURE("DUPLICATED-ALLOCATION-NOTIFICATION");
        break;
    case REDUCE:
        LOG_INFO_PURE("REDUCE");
        break;
    case RESTART:
        LOG_INFO_PURE("RESTART");
        break;
    case EXPIRATION:
        LOG_INFO_PURE("EXPIRATION");
        break;
    default:
        LOG_INFO_PURE((uint16_t) management);
    }

    if(subBlock.getSubBlock().count(true) == 1) {
        for (DSMESABSpecification::SABSubBlock::iterator it = subBlock.getSubBlock().beginSetBits(); it != subBlock.getSubBlock().endSetBits();
                it++) {
            GTS gts = GTS::GTSfromAbsoluteIndex((*it) + subBlock.getSubBlockIndex() * numGTSlots * numChannels, numGTSlots, numChannels,
                    numSuperFramesPerMultiSuperframe);

            LOG_INFO_PURE(" " << gts.slotID << " " << gts.superframeID << " " << (uint16_t)gts.channel);
        }
    }
}

void DSMEPlatformBase::printSequenceChartInfo(IDSMEMessage* imsg) {
    DSMEMessage* msg = static_cast<DSMEMessage*>(imsg);

    IEEE802154eMACHeader &header = msg->getHeader();

    LOG_INFO_PREFIX;

    LOG_INFO_PURE((uint16_t) header.getDestAddr().getShortAddress() << "|");

    LOG_INFO_PURE((uint16_t) header.hasSequenceNumber() << "|");

    LOG_INFO_PURE((uint16_t) header.getSequenceNumber() << "|");

    switch (header.getFrameType()) {
    case IEEE802154eMACHeader::BEACON:
        LOG_INFO_PURE("BEACON");
        break;
    case IEEE802154eMACHeader::DATA:
        LOG_INFO_PURE("DATA");
        break;
    case IEEE802154eMACHeader::ACKNOWLEDGEMENT:
        LOG_INFO_PURE("ACK");
        break;
    case IEEE802154eMACHeader::COMMAND:
    {
        uint8_t cmd = msg->frame->getData()[0];

        switch ((CommandFrameIdentifier) cmd) {
        case ASSOCIATION_REQUEST:
            LOG_INFO_PURE("ASSOCIATION-REQUEST");
            break;
        case ASSOCIATION_RESPONSE:
            LOG_INFO_PURE("ASSOCIATION-RESPONSE");
            break;
        case DISASSOCIATION_NOTIFICATION:
            LOG_INFO_PURE("DISASSOCIATION-NOTIFICATION");
            break;
        case DATA_REQUEST:
            LOG_INFO_PURE("DATA-REQUEST");
            break;
        case BEACON_REQUEST:
            LOG_INFO_PURE("BEACON-REQUEST");
            break;
        case DSME_ASSOCIATION_REQUEST:
            LOG_INFO_PURE("DSME-ASSOCIATION-REQUEST");
            break;
        case DSME_ASSOCIATION_RESPONSE:
            LOG_INFO_PURE("DSME-ASSOCIATION-RESPONSE");
            break;
        case DSME_BEACON_ALLOCATION_NOTIFICATION:
            LOG_INFO_PURE("DSME-BEACON-ALLOCATION-NOTIFICATION");
            break;
        case DSME_BEACON_COLLISION_NOTIFICATION:
            LOG_INFO_PURE("DSME-BEACON-COLLISION-NOTIFICATION");
            break;
        case DSME_GTS_REQUEST:
        case DSME_GTS_REPLY:
        case DSME_GTS_NOTIFY:
        {
            DSMEMessage* m = getLoadedMessage(msg->getSendableCopy());
            m->getHeader().decapsulateFrom(m);

            MACCommand cmdd;
            cmdd.decapsulateFrom(m);
            GTSManagement man;
            man.decapsulateFrom(m);

            switch (cmdd.getCmdId()) {
            case DSME_GTS_REQUEST: {
                LOG_INFO_PURE("DSME-GTS-REQUEST");
                GTSRequestCmd req;
                req.decapsulateFrom(m);
                printDSMEManagement(msg->frame->getData()[1],req.getSABSpec(),cmdd.getCmdId());
                break;
            }
            case DSME_GTS_REPLY: {
                LOG_INFO_PURE("DSME-GTS-REPLY");
                GTSReplyNotifyCmd reply;
                reply.decapsulateFrom(m);
                printDSMEManagement(msg->frame->getData()[1],reply.getSABSpec(),cmdd.getCmdId());
                break;
            }
            case DSME_GTS_NOTIFY: {
                LOG_INFO_PURE("DSME-GTS-NOTIFY");
                GTSReplyNotifyCmd notify;
                notify.decapsulateFrom(m);
                printDSMEManagement(msg->frame->getData()[1],notify.getSABSpec(),cmdd.getCmdId());
                break;
            }
            default:
                break;
            }

            this->releaseMessage(m);

            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        LOG_INFO_PURE("UNKNOWN");
        break;
    }

    LOG_INFO_PURE(cometos::endl);
    return;
}

// TODO: make sure ALL callers check for nullptr (BeaconManager / GTSManager)
IDSMEMessage* DSMEPlatformBase::getEmptyMessage()
{
    dsme_atomicBegin();
    messagesInUse++;
    DSME_ASSERT(messagesInUse <= MSG_POOL_SIZE); // TODO should return nullptr (and check everywhere!!)
    dsme_atomicEnd();

    DSMEMessage* msg = messageBuffer.aquire();
    ASSERT(msg != nullptr);
    DSME_ASSERT(msg->frame && msg->frame.unique());

    msg->receivedViaMCPS = false;
    signalNewMsg(msg);
    return msg;
}

DSMEMessage* DSMEPlatformBase::getLoadedMessage(AirframePtr frame)
{
    dsme_atomicBegin();
    messagesInUse++;
    DSME_ASSERT(messagesInUse <= MSG_POOL_SIZE); // TODO
    dsme_atomicEnd();
    DSMEMessage* msg = messageBuffer.aquire(frame);
    ASSERT(msg != nullptr);
    msg->receivedViaMCPS = false;
    signalNewMsg(msg);
    return msg;
}

void DSMEPlatformBase::releaseMessage(IDSMEMessage* imsg) {
    DSMEMessage* msg = static_cast<DSMEMessage*>(imsg);

    dsme_atomicBegin();
    DSME_ASSERT(msg != nullptr);
    DSME_ASSERT(messagesInUse > 0);

    DSME_ASSERT(!msg->frame || msg->frame.unique());

    messagesInUse--;
    dsme_atomicEnd();

    signalReleasedMsg(msg);
    messageBuffer.release(msg);
    //delete msg;
}

}
