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
 * @author Martin Ringwelski, Andreas Weigel
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "LowpanAdaptionLayer.h"
#include "lowpan-macros.h"
#include "palId.h"
#include "palLocalTime.h"
#include "QueuePacket.h"
#include "IPHCDecompressor.h"
#include "IPHCCompressor.h"
#include "DgOrderedLowpanQueue.h"
#include "FifoLowpanQueue.h"
#include "NoCC.h"
#include "LowpanOrderedForwarding.h"
#include "Task.h"
#include "OutputStream.h"

#include "CsmaMac.h"
#include "pinEventOutput.h"

enum {
     LOWPAN_DATAGRAM_START = 1,
     LOWPAN_DATAGRAM_SUCCESS = 8,
     LOWPAN_DATAGRAM_FAIL = 15
};

using cometos::DataIndication;
using cometos::DataRequest;

namespace cometos_v6 {

class LowpanOrderedForwarding;

const uint16_t LowpanAdaptionLayer::DURATION_EWMA_ONE;
const uint16_t LowpanAdaptionLayer::DURATION_EWMA_ALPHA;
const uint16_t LowpanAdaptionLayer::DURATION_MAX;

Define_Module(LowpanAdaptionLayer);


/*----PUBLIC METHODS----------------------------------------------------------*/

timeOffset_t LowpanAdaptionLayer::getSendDelay() const {
    uint16_t durationVal;
    if (cfg.delayMode == DM_ADAPTIVE) {
        durationVal = stats.avgDuration;
    } else if (cfg.delayMode == DM_RATE) {
        durationVal = fixedAvgPktDuration;
    } else {
        durationVal = 0;
    }

    // calculate delay from duration it takes to send a frame
    time_ms_t minDelay = ((uint16_t)durationVal) * cfg.minDelayNominator / cfg.minDelayDenominator;
    time_ms_t delay = (intrand(durationVal+1) + minDelay) >> LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;
    LOG_DEBUG("nom=" << (int) cfg.minDelayNominator
                     << " denom=" << (int) cfg.minDelayDenominator
                        << " next delay: mode=" << (int) cfg.delayMode
                        << "|avgDur=" << (stats.avgDuration >> LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT)
                        << "|delay=" << delay);
    return delay;
}

LowpanAdaptionLayer::LowpanAdaptionLayer(const char * service_name):
        cometos::RemotelyConfigurableModule<LowpanConfig>(service_name),
        fromIP(this, &LowpanAdaptionLayer::handleIPRequest, "fromIP"),
        fromMAC(this, &LowpanAdaptionLayer::handleMACIndication, "fromMAC"),
        macSnoop(this, &LowpanAdaptionLayer::handleSnoopedLowpanFrame, "macSnoop"),
        toIP(this, "toIP"),
        toMAC(this, "toMAC"),
        fixedAvgPktDuration(DEFAULT_AVG_PKT_DURATION),
        fragmentTagOwn(0),
        queue(nullptr),
        buffer(nullptr),
        fragmentHandler(nullptr),
        fragmentBuffer(nullptr),
        lca(nullptr),
        meshUnder(false),
        sending(false),
        queueStarving(false),
        switchedQo(false),
        numTimesQueueEmpty(0),
        poolToUpper(nullptr),
        tsSend(0),
        lowpanVariant(nullptr),
        nd(NULL)
{
}

LowpanAdaptionLayer::~LowpanAdaptionLayer() {
    deleteHandlerObjects();
}

void LowpanAdaptionLayer::deleteHandlerObjects() {
    // in AssemblyMode, fragmentHandler and fragmentBuffer are the same;
    // make sure to delete only once
    delete fragmentHandler;
    if (fragmentBuffer != fragmentHandler) {
        delete(fragmentBuffer);
    }
    fragmentHandler = nullptr;
    fragmentBuffer = nullptr;

    delete lca;
    lca = nullptr;

    delete queue;
    queue = nullptr;

    delete buffer;
    buffer = nullptr;

    delete lowpanVariant;
    lowpanVariant = nullptr;

    if (poolToUpper != nullptr && poolToUpper != NULL) {
        poolToUpper->finish();
    }
    delete poolToUpper;
    poolToUpper = nullptr;
}

void LowpanAdaptionLayer::createQueueAndLca(const LowpanConfig& cfg) {
    DgOrderedLowpanQueue* dolq = nullptr;
    switch(cfg.queueType) {
        case LowpanConfigConstants::QT_FIFO: {
            queue = new FifoLowpanQueue(cfg.queueSize,
                                        cometos::mac_getMaximumPayload(),
                                        createCallback(&LowpanAdaptionLayer::receiveMACResponse));
            break;
        }
        case LowpanConfigConstants::QT_DG_ORDERED: {
            dolq = new DgOrderedLowpanQueue(cfg.queueSize,
                    cometos::mac_getMaximumPayload(),
                    createCallback(&LowpanAdaptionLayer::receiveMACResponse));
            queue = dolq;
            break;
        }
        default: {
            ASSERT(false);
            break;
        }
    }

#ifdef OMNETPP
    if (cfg.congestionControlType == LowpanConfigConstants::CCT_NONE) {
        lca = static_cast<NoCC*>(getModule("locc"));
        ASSERT(lca != NULL);
    } else {
        LowpanOrderedForwarding* loof = static_cast<LowpanOrderedForwarding*>(getModule("locc"));
        loof->setModulePtrs(this, dolq);
        lca = loof;
        ASSERT(lca != NULL);
    }
#else
    // create local congestion avoider
    if (cfg.congestionControlType == LowpanConfigConstants::CCT_NONE) {
        lca = new NoCC();
    } else {
        ASSERT(cfg.queueType == LowpanConfigConstants::QT_DG_ORDERED);
        ASSERT(dolq != nullptr);
        lca = new LowpanOrderedForwarding(this, dolq);
    }
    lca->manualInit();
#endif

    ASSERT(lca != nullptr);

}

void LowpanAdaptionLayer::createAndInitializeMessagePools(const LowpanConfig& cfg) {
    poolToUpper = new cometos::DynMappedPool<IPv6Request_Content_t>(
                     createCallback(&LowpanAdaptionLayer::initRequestsToUpper),
#ifdef OMNETPP
                     createCallback(&LowpanAdaptionLayer::finishRequestsToUpper),
#endif
                     cfg.numIndicationMsgs);
    poolToUpper->initialize();       // Pool of Messages
}

void LowpanAdaptionLayer::createSpecificationImplementation(const LowpanConfig& cfg) {
    if(cfg.enableLFFR){
        lowpanVariant = new LFFRImplementation(stats,
                                               *fragmentHandler,
                                               *buffer,
#ifdef OMNETPP
                                               BufferSizeVector,
                                               BufferNumElemVector,
#endif
                                               srcContexts,
                                               dstContexts,
                                               _ipRetransmissionList,
                                               this);

        schedule(&_RTOTimer, &LowpanAdaptionLayer::RTOTimeout,
                 RTO_GRANULARITY);
    }
    else {
        lowpanVariant = new Rfc4944Implementation(stats,
                                                  *fragmentHandler,
                                                  *buffer,
#ifdef OMNETPP
                                                  BufferSizeVector,
                                                  BufferNumElemVector,
#endif
                                                  srcContexts,
                                                  dstContexts,
                                                  _ipRetransmissionList,
                                                  this);
    }
}

void LowpanAdaptionLayer::createHandlerObjects(const LowpanConfig& cfg) {
    ASSERT(queue == nullptr);
    ASSERT(poolToUpper == nullptr);
    ASSERT(buffer == nullptr);
    ASSERT(fragmentBuffer == nullptr);
    ASSERT(fragmentHandler == nullptr);
    ASSERT(lowpanVariant == nullptr);
    ASSERT(lca == nullptr);

    LOG_DEBUG("Allocating 6LoWPAN Hanlder objects; "
             << "bufSize=" << cfg.bufferSize
             << "|numReassemblyHandlers=" << (int) cfg.numReassemblyHandlers
             << "|numDatagramHandlers=" << (int) cfg.numDirectDatagramHandlers
             << "|numBufferHandlers=" << (int) cfg.numBufferHandlers
             << "|queueType=" << (int) cfg.queueType
             << "|enableDirectFwd=" << cfg.enableDirectFwd);

    createQueueAndLca(cfg);
    if (cfg.congestionControlType == LowpanConfigConstants::CCT_DG_ORDERED) {
        cometos::CsmaMac* mac = static_cast<cometos::CsmaMac*>(getModule(MAC_MODULE_NAME));
        ASSERT(mac != NULL);
        mac->setPromiscuousMode(true);
    }

    createAndInitializeMessagePools(cfg);

    buffer = new DynLowpanBuffer(cfg.bufferSize, cfg.numBufferHandlers);

    fragmentBuffer = new DynAssemblyBuffer(buffer, cfg.numReassemblyHandlers);
    if (cfg.enableDirectFwd) {
        DirectBuffer* db = new DynDirectBuffer(this, fragmentBuffer, buffer, cfg.numDirectDatagramHandlers, &fragmentTagOwn);
        IpForward * ip = (IpForward *) getModule(IPFWD_MODULE_NAME);
        ASSERT(ip!=NULL);
        db->initialize(ip);
        fragmentHandler = db;
    } else {
        fragmentHandler = fragmentBuffer;
    }

    // create LFFR or vanilla RFC4944 implementation
    createSpecificationImplementation(cfg);

#if defined LOWPAN_ENABLE_BIGBUFFER and defined LOWPAN_USE_STATIC_ALLOCATION
#error "LOWPAN_USE_STATIC_ALLOCATION not yet reimplemented"
    // TODO does not work satisfyingly; setMEntries currently not defined for ManagedBuffer
    ASSERT(false);
    uint8_t     mEntries = LOWPAN_SET_BUFFER_ENTRIES;
    uint16_t    bufferSize = LOWPAN_SET_BUFFER_SIZE;

    CONFIG_NED(mEntries);
    CONFIG_NED(bufferSize);

    buffer->setMEntries(mEntries);
    buffer->setBSize(bufferSize);

    recordScalar("mEntries", mEntries);
    recordScalar("bufferSize", bufferSize);

    if (palId_id() == 0) {
        fragmentBuffer.setMEntries(LOWPAN_SET_ASSEMBLY_ENTRIES);
        recordScalar("assemblyEntries", LOWPAN_SET_ASSEMBLY_ENTRIES);
    } else {
        fragmentBuffer.setMEntries(LOWPAN_SET_ASSEMBLY_ENTRIES_O);
        recordScalar("assemblyEntries", LOWPAN_SET_ASSEMBLY_ENTRIES_O);
    }

#endif
}

void LowpanAdaptionLayer::initialize() {
    RemotelyConfigurableModule<LowpanConfig>::initialize();
    // init specific statistics
    LOG_INFO("Initializing");
    LAL_VECTOR_INI(QueueSizeVector);
    LAL_VECTOR_INI(BufferSizeVector);
    LAL_VECTOR_INI(BufferNumElemVector);
    LAL_VECTOR_INI(FreeIPRequestsVector);
    LAL_VECTOR_INI(MacFrameDurationVector);

    nd = (NeighborDiscovery*) getModule(NEIGHBOR_DISCOVERY_MODULE_NAME);
    ASSERT(nd != NULL);

    // setup remoteAccess
    remoteDeclare(&LowpanAdaptionLayer::getStats, "gs");
    remoteDeclare(&LowpanAdaptionLayer::resetStats, "rs");

    // if there is a ParameterStore, try to load configuration from there
    cometos::ParameterStore * ps = cometos::ParameterStore::get(*this);
    if (ps != NULL) {
        ps->getCfgData(this, cfg);
    }

    // configure via OMNET++ parameters if available
    CONFIG_NED(fixedAvgPktDuration);
    CONFIG_NED(meshUnder);
    CONFIG_NED_OBJ(cfg, minDelayNominator);
    CONFIG_NED_OBJ(cfg, minDelayDenominator);
    CONFIG_NED_OBJ(cfg, macRetryControlMode);
    CONFIG_NED_OBJ(cfg, delayMode);
    CONFIG_NED_OBJ(cfg, enableLFFR);
    CONFIG_NED_OBJ(cfg, bufferSize);
    CONFIG_NED_OBJ(cfg, queueType);
    CONFIG_NED_OBJ(cfg, enableDirectFwd);
    CONFIG_NED_OBJ(cfg, numBufferHandlers);
    CONFIG_NED_OBJ(cfg, numDirectDatagramHandlers);
    CONFIG_NED_OBJ(cfg, numReassemblyHandlers);
    CONFIG_NED_OBJ(cfg, congestionControlType);
    CONFIG_NED_OBJ(cfg, timeoutMs);
    CONFIG_NED_OBJ(cfg, queueSwitchAfter);
    CONFIG_NED_OBJ(cfg, pushBackStaleObjects);
    CONFIG_NED_OBJ(cfg, queueSize);
    CONFIG_NED_OBJ(cfg, useRateTimerForQueueSwitch);

    createHandlerObjects(cfg);

    schedule(&timeOutSchedule, &LowpanAdaptionLayer::fragmentTimeout,
            cfg.timeoutMs);
}

void LowpanAdaptionLayer::finish() {
    lca->finish();

    cancel(&timeOutSchedule);
    cancel(&rateTimer);
    cancel(&_RTOTimer);
    deleteHandlerObjects();

    LAL_SCALAR_REC("drpdPkt_I", dropped_Invalid);
    LAL_SCALAR_REC("drpdPkt_QF", dropped_QueueFull);
    LAL_SCALAR_REC("drpdPkt_MU", dropped_MacUnsuccessful);
    LAL_SCALAR_REC("drpdFrm_FU", dropped_FollowingUnsuccessful);
    LAL_SCALAR_REC("drpdPkt_BF", dropped_BufferFull);
    LAL_SCALAR_REC("drpdPkt_OOH", dropped_BufferOutOfHandlers);
    LAL_SCALAR_REC("drpdPkt_AF", dropped_AssemblyBufferFull);
    LAL_SCALAR_REC("drpdPkt_TF", dropped_TinyBufferFull);
    LAL_SCALAR_REC("drpdFrm_IO", dropped_InvalidOrder);
    LAL_SCALAR_REC("drpdPkt_IOM", dropped_InvalidOrderMissing);
    LAL_SCALAR_REC("drpdPkt_OM", dropped_OutOfMessages);
    LAL_SCALAR_REC("drpdPkt_TO", dropped_Timeout);
    LAL_SCALAR_REC("drpdPkt_NR", dropped_NoRoute);
    LAL_SCALAR_REC("drpdPkt_HL", dropped_HopsLeft);
    LAL_SCALAR_REC("avgDuration", avgDuration);
    LAL_SCALAR_REC("sentFrm", sentFrames);
    LAL_SCALAR_REC("sentPkt", sentPackets);
    LAL_SCALAR_REC("rcvdFrm", receivedFrames);
    LAL_SCALAR_REC("rcvdPkt", receivedPackets);
    LAL_SCALAR_REC("timesQueueEmpty", timesQueueEmpty);
    LAL_SCALAR_REC("objectsPushedBack", objectsPushedBack);
    LAL_SCALAR_REC("timesQueueRecovered", timesQueueRecovered);
    LAL_SCALAR_REC("timesQueueNotRecovered", timesQueueNotRecovered);
}

void LowpanAdaptionLayer::applyConfig(LowpanConfig & newCfg) {
    // now also remove all other objects and recreate them according to new config
    deleteHandlerObjects();
    createHandlerObjects(newCfg);

    cfg = newCfg;
}

bool LowpanAdaptionLayer::isBusy() {
    return poolToUpper->size() != poolToUpper->maxSize();
}

LowpanConfig& LowpanAdaptionLayer::getActive() {
    return cfg;
}


void LowpanAdaptionLayer::initRequestsToUpper(IPv6Request_Content_t * req) {
    req->request.setResponseDelegate(createCallback(&LowpanAdaptionLayer::receiveIPResponse));
    req->buf = NULL;
}

void LowpanAdaptionLayer::finishRequestsToUpper(IPv6Request_Content_t * data) {
    TAKE_MESSAGE(&(data->request));
    if (data->request.data.datagram != NULL) {
        delete data->request.data.datagram;
        data->request.data.datagram = NULL;
    }
}

LowpanAdaptionLayerStats LowpanAdaptionLayer::getStats() {
    return stats;
}


void LowpanAdaptionLayer::resetStats() {
    stats.reset();
}

void LowpanAdaptionLayer::fragmentTimeout(cometos::Message *msg) {
    uint8_t numTimeouts = fragmentHandler->tick();
    LOG_DEBUG("Num timeouted QO: " << (int) numTimeouts);
    LAL_SCALAR_ADD_F(dropped_Timeout, numTimeouts); //dropped_Timeout
    _ipRetransmissionList.tick();
    eraseQueueObject(); // this is for the LFFR _ipRetransmissionList. find a better way

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer->getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer->getNumBuffers());
#endif

    // to prevent starving in case the deleted datagram was at first position
    // in queue, but the queue is not yet empty, we bump the queue again
    sendIfAllowed();

    schedule(msg, &LowpanAdaptionLayer::fragmentTimeout, cfg.timeoutMs);
}


void LowpanAdaptionLayer::handleSnoopedLowpanFrame(LowpanIndication* ind) {

    LowpanFragMetadata fragMeta;
    IPv6Datagram dg;
    cometos::Airframe& frame = ind->getAirframe();

    lowpanVariant->parseLowpanAndIpHeader(frame,
                                          ind->head,
                                          ind->src,
                                          ind->dst,
                                          fragMeta,
                                          dg);

    lca->snoopedFrame(dg, ind->src, fragMeta);
    delete(ind);
}

uint8_t LowpanAdaptionLayer::checkMesh(cometos::Airframe & frame,
                                    uint8_t dispatch,
                                    Ieee802154MacAddress& src,
                                    Ieee802154MacAddress& dst) {
    ASSERT(isMeshHeader(dispatch));
    lowpanVariant->getSrcDstAddrFromMesh(frame, dispatch, src, dst);
    frame >> dispatch;
    return dispatch;
}

void LowpanAdaptionLayer::handleMACIndication(LowpanIndication *ind) {
    Ieee802154MacAddress    src = Ieee802154MacAddress(ind->src);
    Ieee802154MacAddress    dst = Ieee802154MacAddress(ind->dst);
    cometos::Airframe*      frame = ind->decapsulateAirframe();
    cometos::MacRxInfo info;
    uint8_t lowpanDispatch = ind->head;

    uint8_t* data = frame->getData();
    if (data[12] == 0xa1 && data[13] == 0x81) {
       LOG_ERROR("RECV tok81A1");
    }

    LAL_SCALAR_INC(receivedFrames);


    LOG_DEBUG("Rcvd MAC Ind. f:" << src.a4() << " h:" << (int)lowpanDispatch);

    IPv6DatagramInformation  dgInfo;

    if (isMeshHeader(lowpanDispatch)) {
        Ieee802154MacAddress ourAddress = dst;
        lowpanDispatch = checkMesh(*frame, lowpanDispatch, src, dst);
        bool isPacketAddressedToUs = (ourAddress == dst);
        if (!isPacketAddressedToUs) {
            delete(ind);
            return;  // A sublayer should work with the mesh routing.
        }
    }

    if (isBroadcastHeader(lowpanDispatch)) {
        // broadcast NYI
        ASSERT(false);
    }

    lowpanVariant->parseframe(*frame, lowpanDispatch, src, dst, &dgInfo);

    if(dgInfo.isValid()){
        saveIPContextandSendtoIpLayer(dgInfo, src, dst);
    } else {
        // if parseframe did not produce a valid datagram, give back memory
        dgInfo.free();
    }

    delete ind;
    delete frame;
}


bool LowpanAdaptionLayer::isSuccessorOfActiveDg(const QueueObject& obj) {
    bool idValid;
    LocalDgId idcur = queue->getFront()->getDgId(idValid);
    if (!idValid) {
        return false;
    }

    LocalDgId idrcv = obj.getDgId(idValid);
    if (!idValid) {
        return false;
    }

    return (!queue->hasNext())
            && idcur.src == idrcv.src
            && idcur.dst == idrcv.dst
            && idrcv.tag - idcur.tag <= ((uint16_t) -1) / 2;

}

void LowpanAdaptionLayer::sendNextFrame() {
    const IPv6Datagram* dg;
    LowpanFragMetadata fragMeta(0,0,0,0);

    ASSERT(queue->getQueueSize() >= 1);


    // if we are told to send the next frame and the current DG does
    // have nothing to send, we probably have a failure on the path
    // behind us and will eventually try to carry on with the next DG
    if (!queue->hasNext()) {
        if (queue->getQueueSize()>1) {
            LAL_SCALAR_INC(timesQueueEmpty);
        }
#ifdef ENABLE_LOGGING
        bool idValid;
        LocalDgId id = queue->getFront()->getDgId(idValid);
        LOG_DEBUG("DG: src=" << id.src.a4() << "|dst=" << id.dst.a4() << "|tag=" << id.tag);
#endif
        if (numTimesQueueEmpty == 0) {
            LOG_DEBUG("First empty " << palLocalTime_get());
            firstEmpty = palLocalTime_get();
        } else {
            LOG_DEBUG("next empty send after " << palLocalTime_get() - firstEmpty << " ms");
        }

        numTimesQueueEmpty = numTimesQueueEmpty == 0xFF ? 0xFF : numTimesQueueEmpty+1;

        if (numTimesQueueEmpty >= cfg.queueSwitchAfter) {
            LOG_DEBUG("Pushed back current queue object after " << palLocalTime_get() - firstEmpty << " ms" << "; queueSize=" << (int) queue->getQueueSize());
            queue->pushBack();
            // if we switched, we need to inform the LCA about it before we send
            lca->switchedQueueObject();
            if (queue->getQueueSize() > 1) {
                LAL_SCALAR_INC(timesQueueNotRecovered);

                // we start anew with another queue object, therefore, reset the counter
                numTimesQueueEmpty = 0;
            } else {
                // we still have the same starving queue object in queue, do NOT reset
                // the counter here so that the next event will lead pushing back the
                // starved object (unless the events recovers it, of course)
            }
            switchedQo = true;
        } else {
            if (cfg.useRateTimerForQueueSwitch) {
                uint16_t delay = stats.avgDuration >> LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;
                schedule(&rateTimer, &LowpanAdaptionLayer::rateTimerFired, delay);
                LOG_DEBUG("Scheduled " << delay << "ms to wait for predecessor sending");
            }
        }
    } else {
        // we got a frame from the queue, reset emptiness-counter
        numTimesQueueEmpty = 0;
    }

    fragMeta.congestionFlag = lca->getCongestionStatus();

    DataRequest* mrq = queue->getNext(fragMeta, dg);

    if (mrq == NULL) {
        LOG_DEBUG("Got NULL frame queueSize=" << (int) queue->getQueueSize());
    } else {
        if (switchedQo) {
            switchedQo = false;
            LOG_INFO("Switched QO to " << mrq->dst);
        }
        LOG_DEBUG("Sending new fragment " << dg->dst.str() << "|" << dg->src.str());

        if (fragMeta.isFirstOfDatagram()) {
            EVENT_OUTPUT_WRITE(LOWPAN_DATAGRAM_START);
        }

        if (meshUnder) {
            // TODO: Only 16 bit MAC Addresses with a fixed hop limit of 14
            // Check also if the byte order is right...
            uint16_nbo dst = mrq->dst;
            uint16_nbo src = palId_id();
            LOG_DEBUG("Adding Mesh Under Header, f:" << src.getUint16_t() << ", t:" << dst.getUint16_t());
            mrq->getAirframe() << dst << src << (uint8_t)0xBE;
        }

        // inform a local congestion avoidance mechanism about us sending a frame
        lca->sentFrame(fragMeta, Ieee802154MacAddress(mrq->dst));

        sending = true;
        tsSend =  palLocalTime_get();
        toMAC.send(mrq);
    }
}


void LowpanAdaptionLayer::handleIPRequest(IPv6Request *iprequest) {
    BufferInformation* buf = buffer->getCorrespondingBuffer(
            iprequest->data.datagram->getData());
    uint16_t posInBuffer = 0;
    LOG_DEBUG("IP Datagram Datalength: " << iprequest->data.datagram->getUpperLayerPayloadLength());
    LOG_DEBUG("To IP: " << iprequest->data.datagram->dst.str() << " MAC: " << iprequest->data.dstMacAddress.a4());

    if (buf == NULL) {
        ManagedBuffer::MbRequestStatus mbStatus;
        buf = buffer->getBuffer(iprequest->data.datagram->getUpperLayerPayloadLength(), mbStatus);
        if (buf != NULL) {
            buf->copyToBuffer(iprequest->data.datagram->getData(), iprequest->data.datagram->getUpperLayerPayloadLength());
        } else {
            LOG_ERROR("Buffer full " << iprequest->data.datagram->getUpperLayerPayloadLength());
            // no new buffer could be allocated, pin down reason
            if (mbStatus == ManagedBuffer::MbRequestStatus::FAIL_MEMORY) {
                LAL_SCALAR_INC(dropped_BufferFull);
            } else if (mbStatus == ManagedBuffer::MbRequestStatus::FAIL_HANDLERS) {
                LAL_SCALAR_INC(dropped_BufferOutOfHandlers);
            }
        }
    } else {
        posInBuffer = iprequest->data.datagram->getData() - buf->getContent();
        LOG_DEBUG("Forwarding from Assembly Buffer. posInBuffer=" << posInBuffer << " c:" << HEXOUT << (int)buf->getContent()[posInBuffer] << DECOUT);
    }

    if (buf != NULL) {
        // TODO using MAC<-->IPv6 address translation assumption
        QueueObject*  queueObj = lowpanVariant->processBufferSpaceAndReturnQueueObject(
               iprequest, fragmentTagOwn++, buf, posInBuffer);

        if (!enqueueQueueObject(queueObj)) {
            LOG_ERROR("Queue Full");
            LAL_SCALAR_INC(dropped_QueueFull);
            delete queueObj; ///< also sends ipresponse response
        } else {
            LOG_INFO("Enqueued IP Pkt");
        }
    } else {
        LOG_ERROR("No Buffer");
        // no buffer could be allocated
        iprequest->response(new IPv6Response(iprequest, IPv6Response::IPV6_RC_FAIL_ALREADY_COUNTED));
    }

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer->getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer->getNumBuffers());
#endif
}

void LowpanAdaptionLayer::receiveIPResponse(IPv6Response *resp) {
    switch (resp->success){
        case IPv6Response::IPV6_RC_SUCCESS: {
            LOG_DEBUG("IPv6Rsp rcvd");
            break;
        }
        case IPv6Response::IPV6_RC_OUT_OF_UPPER_MSGS: {
            LOG_INFO("Dropped OutOfMessages upper");
            LAL_SCALAR_INC(dropped_OutOfMessages);
            break;
        }
        case IPv6Response::IPV6_RC_OUT_OF_LOWER_MSGS: {
            LOG_INFO("Dropped OutOfMessages lower");
            LAL_SCALAR_INC(dropped_OutOfMessages);
            break;
        }
        case IPv6Response::IPV6_RC_NO_ROUTE: {
            LOG_INFO("Dropped No Route");
            LAL_SCALAR_INC(dropped_NoRoute);
            break;
        }
        default: {
            LOG_INFO("Neg IPv6Rsp rcvd: " << resp->success);
            break;
        }
    }

    IPv6Request * originalReq = resp->refersTo;
    ASSERT(originalReq != NULL);

    TAKE_MESSAGE(originalReq);

    IPv6Request_Content_t * originalData = poolToUpper->find(*originalReq);
    ASSERT(originalData != NULL);
    ASSERT(&(originalData->request) == originalReq);


    LOG_DEBUG("Deleting IPv6Req");
    ASSERT(originalData->request.data.datagram != NULL);

    // clean up and give back message structure
    delete originalData->request.data.datagram;
    originalData->request.data.datagram = NULL;

    if (originalData->buf->isUsed() && resp->freeBuf) {
        originalData->buf->free();
    }
#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer->getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer->getNumBuffers());
#endif

    // in case some metadata has been attached to the original message, cleanup
    originalReq->removeAll();
    poolToUpper->putBack(originalData);

    LAL_VECTOR_REC(FreeIPRequestsVector, (double)(poolToUpper->maxSize() - poolToUpper->size()));
    delete resp;
}

void LowpanAdaptionLayer::receiveMACResponse(cometos::DataResponse *resp) {
    time_ms_t durationLowpan = palLocalTime_get() - tsSend;
    time_ms_t durationMac = 0;

    // we are no longer sending a lowpan frame
    sending = false;

    if (resp->has<cometos::MacTxInfo>()) {
        durationMac = resp->get<cometos::MacTxInfo>()->txDuration;
    }

    time_ms_t duration = durationMac == 0 ? durationLowpan : durationMac;

    // this section assumes uint16_t for stats.avgDuration
    LAL_VECTOR_REC(MacFrameDurationVector, duration);
    if (duration < LowpanAdaptionLayerStats::DURATION_FACTOR && resp->success) {
        if (stats.avgDuration == 0) {
            stats.avgDuration = duration << LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;
        } else {
            stats.avgDuration = (((uint32_t) stats.avgDuration) * DURATION_EWMA_ALPHA
                    + ((duration << LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT) * (DURATION_EWMA_ONE - DURATION_EWMA_ALPHA))) / DURATION_EWMA_ONE;
        }
    }

    if (resp->success) {
        LAL_SCALAR_INC(sentFrames);
        LOG_INFO("low->MAC succ");
    } else {
        LAL_SCALAR_INC(dropped_MacUnsuccessful);
        LOG_WARN("low->MAC unsucc");
        EVENT_OUTPUT_WRITE(LOWPAN_DATAGRAM_FAIL);
    }

    bool preventRateRestriction = false;

    // check if last frame sent was sent to final destination of datagram
    if (queue->getQueueSize() > 0) {
        const QueueObject* lastsent = queue->getObjectToRespondTo();
        if (!isnull(lastsent)) {
            const IPv6Datagram* lastdg = lastsent->getDg();
            if (!isnull(lastdg)) {
                const Ieee802154MacAddress* finalDst = nd->findNeighbor(lastsent->getDg()->dst);
                if (finalDst != NULL &&
                        *finalDst == lastsent->getDstMAC()) {
                    preventRateRestriction = true;
                    LOG_DEBUG("avoid rate restriction");
                }
            }
        }
    }

    switch (queue->response(resp)) {
    case LOWPANQUEUE_PACKET_FINISHED:
        LAL_SCALAR_INC(sentPackets);
        EVENT_OUTPUT_WRITE(LOWPAN_DATAGRAM_SUCCESS);
        LOG_DEBUG("Packet finished");
        break;
    case LOWPANQUEUE_ERROR:
        LOG_ERROR("No corresponding QueueObject found!");
        break;
    case LOWPANQUEUE_IN_PROGRESS:
        LOG_DEBUG("QueueObject in progress");
        break;
    case LOWPANQUEUE_PACKET_DROPPED:
        lca->abortCurrent();
        LOG_INFO("QueueObject dropped");
        break;
    default:
        LOG_ERROR("Unexpected Return Value");
    }

    if (!preventRateRestriction
            && (cfg.delayMode == DM_RATE || cfg.delayMode == DM_ADAPTIVE)) {
        timeOffset_t d = getSendDelay();
        LOG_DEBUG("schedule sendNext in " << d);
        schedule(&rateTimer, &LowpanAdaptionLayer::rateTimerFired, d);
    } else {
        sendIfAllowed();
    }
}

bool LowpanAdaptionLayer::enqueueQueueObject(QueueObject * obj) {
    bool ret = queue->add(obj);
    bool idValid;
    // if the incoming fragment
    // is actually a successor of the current one and the queue is already
    // starving, we decide to push back the curr element in favor of the
    // next one
    // TODO should probably be moved to LowpanOrderedForwarding;
    if (isSuccessorOfActiveDg(*obj) && cfg.pushBackStaleObjects) {
#ifdef ENABLE_LOGGING
        LocalDgId id = queue->getFront()->getDgId(idValid);
        LocalDgId id2 = obj->getDgId(idValid);
        LOG_DEBUG("Pushed back stale queue object: src="
                     << id.src.a4() << "|dst=" << id.dst.a4() << "|tag=" << (int) id.tag
                     << " because of: src="
                     << id2.src.a4() << "|dst=" << id2.dst.a4() << "|tag=" << (int) id2.tag);
#endif
        lca->switchedQueueObject();
        queue->pushBack();
        if (queue->getQueueSize() > 1) {
            LAL_SCALAR_INC(objectsPushedBack);
        }
    } else if (queue->hasNext() && numTimesQueueEmpty > 0) {
        if (cfg.useRateTimerForQueueSwitch && isScheduled(&rateTimer)) {
            cancel(&rateTimer);
        }
        if (queue->getQueueSize() > 1) {
            LAL_SCALAR_INC(timesQueueRecovered);
        }
        queueStarving = false;
    } else {
        // nothing.
    }

    LAL_VECTOR_REC(QueueSizeVector, queue->getQueueSize());
    if (ret) {
        obj->logEnqueue();
        LocalDgId id = obj->getDgId(idValid);
        ASSERT(idValid);
        lca->enqueueFrame(*(obj->getDg()), id);

        LOG_DEBUG("New object in queue, try to send.");
        sendIfAllowed();
    }

    return ret;
}

void LowpanAdaptionLayer::sendIfAllowed() {
    ENTER_METHOD_SILENT();
    LOG_DEBUG("Trying to send; rtSched=" << (int) isScheduled(&rateTimer)
            << "|lcaAllowed=" << (int) lca->sendingAllowed()
            << "|notSending=" << (int)  !sending
            << "|queueSize=" << (int)  queue->getQueueSize());
//    std::cout << "send if allowed... ";
    if (!isScheduled(&rateTimer)
            && lca->sendingAllowed()
            && !sending
            && queue->getQueueSize() > 0) {
//        std::cout << "OK ";
        sendNextFrame();
    } else {
//        std::cout << "NO ";
    }
}

void LowpanAdaptionLayer::eraseQueueObject() {
    const QueueObject* qo = queue->getFront();
    queue->removeObjects();

    // if the active queue object was removed due to a timeout,
    // inform the congestion mechanism about the switch
    if (qo != queue->getFront()) {
        lca->switchedQueueObject();
    }

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer->getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer->getNumBuffers());
#endif
}

void LowpanAdaptionLayer::rateTimerFired(cometos::Message * msg) {

    LOG_DEBUG("sendNext");
    cancel(&rateTimer);
    sendIfAllowed();

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LAL_VECTOR_REC(BufferSizeVector, (double)buffer->getUsedBufferSize());
    LAL_VECTOR_REC(BufferNumElemVector, (double)buffer->getNumBuffers());
#endif
}

BufferInformation* LowpanAdaptionLayer::getLowpanDataBuffer(uint16_t size) {
    ManagedBuffer::MbRequestStatus status;
    BufferInformation * data = buffer->getBuffer(size, status);

    if (status == ManagedBuffer::MbRequestStatus::SUCCESS) {
        ASSERT(data != NULL);
        return data;
    } else if (status == ManagedBuffer::MbRequestStatus::FAIL_MEMORY) {
        stats.dropped_BufferFull++;
        return NULL;
    } else if (status == ManagedBuffer::MbRequestStatus::FAIL_HANDLERS) {
        stats.dropped_BufferOutOfHandlers++;
        return NULL;
    }
    return NULL;
}

BufferInformation* LowpanAdaptionLayer::getContainingLowpanDataBuffer(const uint8_t* ptr) {
    return buffer->getCorrespondingBuffer(ptr);
}

void LowpanAdaptionLayer::RTOTimeout(cometos::Message *msg) {
    _ipRetransmissionList.decrementRTOTick();
    schedule(&_RTOTimer, &LowpanAdaptionLayer::RTOTimeout, RTO_GRANULARITY);
}

void LowpanAdaptionLayer::saveIPContextandSendtoIpLayer(
        IPv6DatagramInformation & dgInfo,
        Ieee802154MacAddress& src, Ieee802154MacAddress& dst) {
    IPv6Request_Content_t * reqData = poolToUpper->get();
    if (reqData != NULL) {
        reqData->request.data.datagram = dgInfo.datagram;
        dgInfo.datagram = NULL;
        if(dgInfo.rxInfo != NULL){
            if (dgInfo.rxInfo->isValid()) {
                reqData->request.set(dgInfo.rxInfo);
                dgInfo.rxInfo = NULL;
            } else {
                delete dgInfo.rxInfo;
                dgInfo.rxInfo = NULL;
            }
        }
        reqData->buf = dgInfo.buf;
        dgInfo.buf = NULL;
        reqData->request.data.dstMacAddress = dst;
        reqData->request.data.srcMacAddress = src;
        LOG_DEBUG("Packet from " << reqData->request.data.datagram->src.getAddressPart(7)
                    << " to " << reqData->request.data.datagram->dst.getAddressPart(7));
//#ifdef OMNETPP
//        FollowingHeader* fh = reqData->request.data.datagram->getNextHeader();
//        while (fh != NULL) {
//            LOG_DEBUG("Next Header Type: " << (int)fh->getHeaderType() << " DataLength: " << (int)fh->getUpperLayerPayloadLength());
//            fh = fh->getNextHeader();
//        }
//#endif
        LAL_SCALAR_INC(receivedPackets);
// Rem This Geogin
#ifndef ENABLE_TESTING
        toIP.send(&(reqData->request));
#endif
    } else {
        LAL_SCALAR_INC(dropped_OutOfMessages);
        LOG_ERROR("No ContentReq");

    }
}

} // namespace cometos_v6

namespace cometos {

uint8_t mac_getMaximumPayload() {
    // 81 is the worst case. TODO: How to get the actual value?
    return 81;
}


void serialize(ByteVector & buf, const cometos_v6::LowpanAdaptionLayerStats & val) {
    serialize(buf, val.dropped_Invalid);
    serialize(buf, val.dropped_QueueFull);
    serialize(buf, val.dropped_MacUnsuccessful);
    serialize(buf, val.dropped_FollowingUnsuccessful);
    serialize(buf, val.dropped_BufferFull);
    serialize(buf, val.dropped_BufferOutOfHandlers);
    serialize(buf, val.dropped_AssemblyBufferFull);
    serialize(buf, val.dropped_TinyBufferFull);
    serialize(buf, val.dropped_InvalidOrder);
    serialize(buf, val.dropped_OutOfMessages);
    serialize(buf, val.dropped_Timeout);
    serialize(buf, val.dropped_NoRoute);
    serialize(buf, val.dropped_HopsLeft);

    serialize(buf, val.avgDuration);

    serialize(buf, val.sentFrames);
    serialize(buf, val.sentPackets);
    serialize(buf, val.receivedFrames);
    serialize(buf, val.receivedPackets);
    serialize(buf, val.timesQueueEmpty);
    serialize(buf, val.objectsPushedBack);
    serialize(buf, val.timesQueueRecovered);
    serialize(buf, val.timesQueueNotRecovered);
}
void unserialize(ByteVector & buf, cometos_v6::LowpanAdaptionLayerStats & val) {
    unserialize(buf, val.timesQueueNotRecovered);
    unserialize(buf, val.timesQueueRecovered);
    unserialize(buf, val.objectsPushedBack);
    unserialize(buf, val.timesQueueEmpty);
    unserialize(buf, val.receivedPackets);
    unserialize(buf, val.receivedFrames);
    unserialize(buf, val.sentPackets);
    unserialize(buf, val.sentFrames);

    unserialize(buf, val.avgDuration);

    unserialize(buf, val.dropped_HopsLeft);
    unserialize(buf, val.dropped_NoRoute);
    unserialize(buf, val.dropped_Timeout);
    unserialize(buf, val.dropped_OutOfMessages);
    unserialize(buf, val.dropped_InvalidOrder);
    unserialize(buf, val.dropped_TinyBufferFull);
    unserialize(buf, val.dropped_AssemblyBufferFull);
    unserialize(buf, val.dropped_BufferOutOfHandlers);
    unserialize(buf, val.dropped_BufferFull);
    unserialize(buf, val.dropped_FollowingUnsuccessful);
    unserialize(buf, val.dropped_MacUnsuccessful);
    unserialize(buf, val.dropped_QueueFull);
    unserialize(buf, val.dropped_Invalid);
}

} // namespace cometos
