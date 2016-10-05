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

#include "TimeSyncService.h"
#include "MacAbstractionBase.h"
#include "palId.h"
#include "palLed.h"
#include "palLocalTime.h"
#include "OverwriteAddrData.h"
#include "MacControl.h"
#include "ForwardMacMeta.h"


namespace cometos {

const timeOffset_t TimeSyncService::SYNC_TIMEOUT;
const timeOffset_t TimeSyncService::REFRESH_INTERVAL;
const timeOffset_t TimeSyncService::NUM_MISSING_MSGS_ACCEPTED;
const uint8_t TimeSyncService::NOT_SYNC_DEPTH;
const char * const TimeSyncService::MODULE_NAME = "tss";

Define_Module(TimeSyncService);


namespace ts {

time_ms_t getTimestamp(Message * msg) {
    ASSERT(msg != NULL);
    time_ms_t ts;

    // check if the packet was timestamped at reception, else timestamp packet
    // now as fallback, which can be arbitrarily more inaccurate
    MacTxInfo * info = msg->has<MacTxInfo>() ? msg->get<MacTxInfo>() : NULL;
    LOG_DEBUG("info=" << (uintptr_t) info);
    if (info != NULL && info->tsInfo.isValid) {
        LOG_DEBUG("MetaData ts=" << info->tsInfo.ts);
        ts = info->tsInfo.ts;
    } else {
        ts = palLocalTime_get();
        LOG_DEBUG("Default ts=" << ts);
    }
    return ts;
}

}


TimeSyncService::TimeSyncService(const char * service_name, uint8_t accuracyMs, uint16_t iMin, uint8_t iMax, uint8_t k, bool dontResetTrickle) :
        TrickleModule(service_name, iMin, iMax, k),
        NUM_REFRESH_TIMEOUTS(((((uint32_t) 1) << iMax) * iMin * NUM_MISSING_MSGS_ACCEPTED - 1) / REFRESH_INTERVAL + 1),
        gateTimestampIn(this, &TimeSyncService::handleTimestampMsg, "gateTimestampIn"),
        gateTimestampOut(this, "gateTimestampOut"),
        gateInitialIn(this, &TimeSyncService::handleInitialMsg, "gateInitialIn"),
        gateInitialOut(this, "gateInitialOut"),
        refreshTimeoutCount(0),
        syncData(),
        isTransmitting(false),
        currSyncSrc(MAC_BROADCAST),
        currTsData(),
        accuracyMs(accuracyMs),
        noReset(dontResetTrickle)
{
}


void TimeSyncService::initialize() {
    TrickleModule::initialize();
    bool startAtInit = false;
    bool initiallyIsMaster = false;
    CONFIG_NED(startAtInit);
    CONFIG_NED(initiallyIsMaster);
    syncData.depth = initiallyIsMaster ? 0 : NOT_SYNC_DEPTH;

    if (startAtInit) {
        RoleCfg asMaster(initiallyIsMaster);
        start(asMaster);
    }
    remoteDeclare(&TimeSyncService::stop, "stop");
    remoteDeclare(&TimeSyncService::start, "start");
    remoteDeclare(&TimeSyncService::getInfo, "gnt");
}

void TimeSyncService::finish() {
    TrickleModule::finish();
    cancel(&refreshTimeoutMsg);
    cancel(&packetTimeoutMsg);
}


void TimeSyncService::handleTimestampMsg(DataIndication *msg) {
    LOG_INFO("tsMsg: src=" << msg->src << "|syncSrc=" << (int) currSyncSrc  << "|active=" << (int) isActive() << "|syncPending=" << (int)syncPending());
    TimeSyncData tsd;

    if (msg->getAirframe().getLength() < sizeof(tsd.ts) + sizeof(tsd.depth)) {
        delete(msg);
        return;
    }

    msg->getAirframe() >> tsd.ts >> tsd.depth;

    LOG_INFO("rcvd tsMsg: myDepth=" << (int) syncData.depth  << "|tsDepth=" << (int) tsd.depth << "|tsTs=" << tsd.ts);
    if (isActive() && syncPending() && msg->src == currSyncSrc) {
        cancel(&packetTimeoutMsg);

        // remember old data to decide if anything new happened
        MasterTime old = syncData;

        // received valid second sync message, check if we use the timestamp
        if (tsd.depth < syncData.depth) {
            // update time sync data
            syncData.depth = tsd.depth + 1;
            syncData.tsLocal = currTsData.ts;
            syncData.tsMaster = tsd.ts;

            NetworkTime::setOffset(((networkTimestamp_t) syncData.tsMaster)
                                 - ((networkTimestamp_t) syncData.tsLocal));

            palLed_on(4);

            LOG_INFO("resynced; tsMaster=" << syncData.tsMaster << "|tsLocal=" << syncData.tsLocal << "|nwkTimeThen=" << NetworkTime::localToNetworkTime(syncData.tsLocal) << "|nwkTimeNow=" << NetworkTime::get());
            // reset reschedule refresh timeout
            refreshTimeoutCount = 0;
            reschedule(&refreshTimeoutMsg, &TimeSyncService::refreshTimeout, REFRESH_INTERVAL);

            // only inform trickle if we received a fresh timestamp
            // (set inconsistency if old ts is significantly worse)
            bool isInconsistent = old.isLessFreshThan(syncData, accuracyMs);
            if (isInconsistent) {
                LOG_INFO("New information, resetting Trickle");
                currTsData.numResets++;
            }
            transmissionReceived_(!isInconsistent || noReset);
        }
    }
    delete(msg);
}

void TimeSyncService::handleInitialMsg(DataIndication * msg) {
    uint8_t depth;

    if (msg->getAirframe().getLength() < sizeof(depth)) {
        delete(msg);
        return;
    }
    msg->getAirframe() >> depth;
    uint8_t pendingDepth = currTsData.depth;
    LOG_INFO("Initial: " << msg->src << "|" << (int)syncData.depth << "|" << (int) depth << "|" << (int) pendingDepth);
    if (isActive() && (!syncPending() || pendingDepth > depth) && !isMaster()) {
        currSyncSrc = msg->src;
        currTsData.depth = depth;


        currTsData.ts = ts::getTimestamp(msg);
        LOG_DEBUG("RxTs: " << currTsData.ts);

        reschedule(&packetTimeoutMsg, &TimeSyncService::packetTimeout, SYNC_TIMEOUT);
    }

    if (depth == NOT_SYNC_DEPTH && isSync()) {

        // we heard a node which is not yet synchronized, send out a beacon
        transmit();
    }

    delete(msg);
}

void TimeSyncService::handleInitialResponse(DataResponse * resp) {
    bool success = resp->success;

    if (success && isSync()) {
        time_ms_t txTs = ts::getTimestamp(resp);
        Airframe * frame = new Airframe();

        time_ms_t txTsMaster = NetworkTime::localToNetworkTime(txTs);
        LOG_ERROR("S:ts=" << txTsMaster << "|d=" << (int) syncData.depth);
        // now put the corresponding master time into the packet
        (*frame) << syncData.depth << txTsMaster;

        LOG_INFO("Tx initial; tsLocal=" << txTs << "|tsSent=" << txTsMaster << " send ts");

        DataRequest * req = new DataRequest(MAC_BROADCAST, frame, createCallback(&TimeSyncService::handleTimestampResponse));
        gateTimestampOut.send(req);
        currTsData.numTx++;
    } else {
        isTransmitting = false;
        LOG_WARN("Omitting second packet (isSync=" << isSync() << ")");
    }

    delete(resp); resp = NULL;
}

void TimeSyncService::handleTimestampResponse(DataResponse * resp) {
    isTransmitting = false;
    delete(resp);
}


bool TimeSyncService::start(RoleCfg & cfg) {
#ifdef OMNETPP
    Enter_Method_Silent();
#endif
    LOG_INFO("Start; asMaster=" << cfg.asMaster);
    if (isScheduled(&packetTimeoutMsg)) {
        cancel(&packetTimeoutMsg);
    }

    if (isScheduled(&refreshTimeoutMsg)) {
        cancel(&refreshTimeoutMsg);
    }

    currSyncSrc = MAC_BROADCAST;

    LOG_ERROR("start");

    if (cfg.asMaster) {
        syncData.depth = 0;
        bool result = start_();
        if (result) {
            reset_();
            return true;
        } else {
            return false;
        }
    } else {
        syncData.depth = NOT_SYNC_DEPTH;
        return start_();
    }
}

bool TimeSyncService::stop() {
    syncData.depth = NOT_SYNC_DEPTH;
    palLed_off(4);
    return stop_();
}


void TimeSyncService::refreshTimeout(Message * msg) {
    if (refreshTimeoutCount > NUM_REFRESH_TIMEOUTS) {
        LOG_WARN("Rf TO");
        syncData.depth = NOT_SYNC_DEPTH;
        palLed_off(4);
        refreshTimeoutCount = 0;
    } else {
        refreshTimeoutCount++;
        reschedule(&refreshTimeoutMsg, &TimeSyncService::refreshTimeout, REFRESH_INTERVAL);
    }
}

void TimeSyncService::packetTimeout(Message * msg) {
    // do nothing, the fact that the timer is no longer scheduled suffices
}

#if 0
time_ms_t TimeSyncService::getMasterTime(time_ms_t ts) {
    if (isMaster()) {
        return ts;
    } else {
        return syncData.tsMaster + ts - syncData.tsLocal;
    }
}
#endif

bool TimeSyncService::isSync() {
    return syncData.depth != NOT_SYNC_DEPTH;
}

TimeSyncData TimeSyncService::getInfo() {
    TimeSyncData ts(syncData.depth, syncData.tsMaster);
    ts.numTx = currTsData.numTx;
    ts.numResets = currTsData.numResets;
    return ts;
}

inline bool TimeSyncService::isMaster() {
    return syncData.depth == 0;
}

inline bool TimeSyncService::syncPending() {
    return isScheduled(&packetTimeoutMsg);
}


void TimeSyncService::transmit() {
    LOG_DEBUG("TxTimer fired");
    if (isTransmitting) {
        return;
    }
    LOG_DEBUG("Transmitting initial " << (int) syncData.depth);

    Airframe * frame = new Airframe();
    (*frame) << syncData.depth;
    DataRequest * req = new DataRequest(MAC_BROADCAST, frame, createCallback(&TimeSyncService::handleInitialResponse));
    gateInitialOut.send(req);
    currTsData.numTx++;
}



void serialize(ByteVector & buf, RoleCfg const & val) {
    serialize(buf, val.asMaster);
}
void unserialize(ByteVector & buf, RoleCfg & val) {
    unserialize(buf, val.asMaster);
}

void serialize(ByteVector & buf, TimeSyncData const & val) {
    serialize(buf, val.depth);
    serialize(buf, val.numTx);
    serialize(buf, val.ts);
    serialize(buf, val.numResets);
}
void unserialize(ByteVector & buf, TimeSyncData & val) {
    unserialize(buf, val.numResets);
    unserialize(buf, val.ts);
    unserialize(buf, val.numTx);
    unserialize(buf, val.depth);
}







///////////////////////////////////////////////////////////////////////////////
/// TimeSyncWirelessBridge           //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


const uint8_t TimeSyncWirelessBridge::QUEUE_LEN;
const uint8_t TimeSyncWirelessBridge::PROTOCOL_PATH_MAX_LEN;

TimeSyncWirelessBridge::TimeSyncWirelessBridge(
        const ByteVector& protocolPathInitial,
        const ByteVector& protocolPathTs,
        const ByteVector& protocolPathRa,
        const char * name) :
    Layer(name),
    fromSerial(this, &TimeSyncWirelessBridge::handleIndicationFromSerial, "fromSecondary"),
    toSerial(this, "toSecondary"),
    gateSnoopIndIn(this, &TimeSyncWirelessBridge::handleIndication, "gateSnoopIndIn"),
    protocolPathInitial(protocolPathInitial),
    protocolPathTs(protocolPathTs),
    protocolPathGateway(protocolPathRa),
    wirelessToSerial(MAC_BROADCAST, &TimeSyncWirelessBridge::responseWirelessToSerial),
    serialToWireless(MAC_BROADCAST, &TimeSyncWirelessBridge::responseSerialToWireless)

{
    wirelessToSerial.syncTimeout.setCallback(CALLBACK_MET(&TimeSyncWirelessBridge::timeout, *this));
    wirelessToSerial.syncTimeout.load(&wirelessToSerial);
    serialToWireless.syncTimeout.setCallback(CALLBACK_MET(&TimeSyncWirelessBridge::timeout, *this));
    serialToWireless.syncTimeout.load(&serialToWireless);
}


void TimeSyncWirelessBridge::initialize() {
    Layer::initialize();
}


void TimeSyncWirelessBridge::handleIndicationFromSerial(DataIndication * msg) {
    uint8_t tmp[PROTOCOL_PATH_MAX_LEN];
    uint8_t i;
    bool isForMe = checkProtocolPathInAirframe(msg->getAirframe(), protocolPathGateway, tmp, i);
    writeBackToAirframe(msg->getAirframe(), tmp, i);
    if (isForMe) {
        // sent to "stack" residing on the gateway (e.g., to use RemoteAccess)
        gateIndOut.send(msg);
    } else {
        msg->remove<MacRxInfo>();
        handle(msg, serialToWireless, gateReqOut);
    }
}


void TimeSyncWirelessBridge::handleIndication(DataIndication * msg) {
    handle(msg, wirelessToSerial, toSerial);
}

void TimeSyncWirelessBridge::handleRequest(DataRequest* msg) {
    toSerial.send(msg);
}

void TimeSyncWirelessBridge::handle(
        DataIndication * msg,
        TimeSyncTransitData& tstd,
        OutputGate<DataRequest>& outputGate)
{
    // queue to send to other interface
    if (!tstd.queue.full()) {
        tstd.queue.push(msg);
        if (tstd.queue.getSize() == 1) {
            sendNext(tstd, outputGate);
        }
    } else {
        getCout() << "QUEUE FULL" << endl;
        delete msg;
    }
}

void TimeSyncWirelessBridge::responseWirelessToSerial(DataResponse* resp) {
    handleResponse(resp, wirelessToSerial, toSerial);
}

void TimeSyncWirelessBridge::responseSerialToWireless(DataResponse* resp) {
    DataIndication* msg = serialToWireless.queue.front();

    // if a response was requested by the original sender, we put the
    // information contained in the MAC response to be serialized and send
    // back to him
    if (msg->has<RequestResponseOverSerial>()) {

        // we do not request a response to this "response" request message
        DataRequest* responseViaSerial = new DataRequest(msg->src, new Airframe());
        responseViaSerial->set(new SerialResponse(msg->get<RequestResponseOverSerial>()->seq, resp->success));
        responseViaSerial->set(resp->get<MacTxInfo>()->getCopy());

        // we rely on the SerialComm queue here and bypass the ordinary message
        // queue, to allow serial response to overtake regular data messages
        toSerial.send(responseViaSerial);
    }
    handleResponse(resp, serialToWireless, gateReqOut);
}

void TimeSyncWirelessBridge::handleResponse(
        DataResponse* resp,
        TimeSyncTransitData& tstd,
        OutputGate<DataRequest>& outputGate)
{
    if (tstd.justSentInitialMsg && resp->has<MacTxInfo>()) {
        MacTxInfo* txi = resp->get<MacTxInfo>();
        ASSERT(txi->tsInfo.isValid);
        ASSERT(txi->tsInfo.ts >= tstd.ts);
        tstd.justSentInitialMsg = false;
        tstd.delta = txi->tsInfo.ts - tstd.ts;
//        getCout() << "tsd=" << tstd.delta << cometos::endl;
    }

    // now we actually remove the message from queue and delete it
    DataIndication* msg = tstd.queue.front();
    tstd.queue.pop();
    delete(msg);

    if (!tstd.queue.empty()) {
        sendNext(tstd, outputGate);
    }

    delete resp;
}


void TimeSyncWirelessBridge::sendNext(TimeSyncTransitData& tstd, OutputGate<DataRequest>& outputGate) {
    if (tstd.queue.empty()) {
        LOG_WARN("QE on sendNext");
        return;
    }

    // get current msg from queue, it is removed later;
    // this is important, because handleRequest checks if queue.size
    // equals one when deciding whether to send the next packet;
    // should probably also be refactored
    DataIndication* msg = tstd.queue.front();
    bool rxInfoPresent = msg->has<MacRxInfo>();
    bool discard = false;
    if (rxInfoPresent && msg->get<MacRxInfo>()->tsInfo.isValid) {
        bool initial = checkForInitialMessage(msg, tstd, discard);
        if (!initial) {
            if (checkForAndChangeTimestampMessage(msg, tstd, discard)) {
            }
        } else {
        }
    }

    if (!discard) {
        OverwriteAddrData* meta = new OverwriteAddrData(msg->src, msg->dst);
        Airframe* frame = msg->decapsulateAirframe();
        DataRequest* req = new DataRequest(
                                            msg->dst,
                                            frame,
                                            createCallback(tstd.handler));

        // possible cases:
        //   1. attach MacRxInfo from MAC to be serialized in SerialComm to be
        //      reissued at receiving side's DataIndication
        //   2. attach MacControl for MAC, which was issued at original sender and
        //      serialized by SerialComm to be reissued here
        // if meta data is not present, nullptr will be set, which simply does nothing
        req->set(msg->unset<MacControl>());
        req->set(msg->unset<MacRxInfo>());

        req->set(meta);
        outputGate.send(req);
    } else {
        tstd.queue.pop();
        delete(msg);
        sendNext(tstd, outputGate);
    }
}

void TimeSyncWirelessBridge::timeout(TimeSyncTransitData* tstd) {
    resetSyncProcess(*tstd);
}


bool TimeSyncWirelessBridge::checkProtocolPathInAirframe(
        Airframe& frame,
        const ByteVector& protocolPath,
        uint8_t tmp[],
        uint8_t& i)
{
    uint8_t len = protocolPath.getSize();
    ASSERT(len <= PROTOCOL_PATH_MAX_LEN);
    bool match = true;

    for (i = 0; i<len; i++) {
        // instead of directly accessing the array behind the frame, we use
        // shifting operator to be independent of changes of the writing order there
        frame >> tmp[i];
        if (tmp[i] != protocolPath[i]) {
            match = false;
            // increase i to get consistent value for writing back the frame
            i++;
            break;
        }
    }

    return match;
}

bool TimeSyncWirelessBridge::checkForInitialMessage(
        DataIndication* msg,
        TimeSyncWirelessBridge::TimeSyncTransitData& tstd,
        bool& discardMessage)
{
    uint8_t tmp[PROTOCOL_PATH_MAX_LEN];
    Airframe& frame = msg->getAirframe();
    uint8_t i;

    bool match = checkProtocolPathInAirframe(frame, protocolPathInitial, tmp, i);
    if (match) {
        uint8_t depth;
        frame >> depth;
        if (tstd.pendingDepth > depth) {
            tstd.currSyncSrc = msg->src;
            tstd.pendingDepth = depth;
            ASSERT(msg->get<MacRxInfo>()->tsInfo.isValid);
            tstd.ts = msg->get<MacRxInfo>()->tsInfo.ts;
            tstd.justSentInitialMsg = true;
            getScheduler().replace(tstd.syncTimeout, TimeSyncService::SYNC_TIMEOUT);
            discardMessage = false;
        } else {
            discardMessage = true;
        }
        frame << depth;
    }


    writeBackToAirframe(frame, tmp, i);
    // frame is back in its original shape, return
    return match;
}

void TimeSyncWirelessBridge::writeBackToAirframe(
        Airframe& frame,
        const uint8_t tmp[],
        const uint8_t& i)
{
    ASSERT(i <= PROTOCOL_PATH_MAX_LEN);
    // write back data to airframe; i is one higher than highest index here
    // use k=i and as index k-1 to be able to abort at 0 for UNSIGNED int
    for (uint8_t k = i; k > 0; k--) {
        frame << tmp[k-1];
    }
}

void TimeSyncWirelessBridge::resetSyncProcess(TimeSyncTransitData& tstd) {
    tstd.currSyncSrc = MAC_BROADCAST;
    tstd.pendingDepth = TimeSyncService::NOT_SYNC_DEPTH;
    tstd.justSentInitialMsg = false;
}

bool TimeSyncWirelessBridge::checkForAndChangeTimestampMessage(
        DataIndication* msg,
        TimeSyncTransitData& tstd,
        bool& discardMessage)
{
    uint8_t tmp[PROTOCOL_PATH_MAX_LEN];
    Airframe& frame = msg->getAirframe();
    uint8_t i;

    bool match = checkProtocolPathInAirframe(frame, protocolPathTs, tmp, i);
    if (match) {
        if (msg->src == tstd.currSyncSrc) {
            getScheduler().remove(tstd.syncTimeout);
            discardMessage = false;
            uint8_t depth;
            time_ms_t timestamp;
            frame >> timestamp;
            frame >> depth;
//            getCout() << "ETS:ts=" << timestamp << "|tsd=" << tstd.delta;
            timestamp += tstd.delta;
//            getCout() << "|tsnew=" << timestamp << cometos::endl;
            frame << depth;
            frame << timestamp;
            resetSyncProcess(tstd);
        } else {
            // we are not currently synchronizing
            discardMessage = true;
        }
    } else {
        // leave message unchanged
    }

    writeBackToAirframe(frame, tmp, i);

    return match;
}


} /* namespace cometos */
