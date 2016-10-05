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

#ifndef TIMESYNCSERVICE_H_
#define TIMESYNCSERVICE_H_

#include "TrickleModule.h"
#include "Endpoint.h"
#include "Layer.h"
#include "Serializable.h"
#include "NetworkTime.h"
#include "Airframe.h"
#include "LoadableTask.h"
#include "MacAbstractionBase.h"
#include "Queue.h"

namespace cometos {

namespace ts {
    time_ms_t getTimestamp(Message * msg);
}

class TimeSyncData {
public:
    TimeSyncData(uint8_t depth = 0xFF, time_ms_t ts=0) :
        depth(depth),
        ts(ts),
        numTx(0),
        numResets(0)
    {}

    uint8_t depth;
    time_ms_t ts;
    uint32_t numTx;
    uint16_t numResets;

};


struct MasterTime {
    MasterTime() :
        tsLocal(0),
        tsMaster(0),
        depth(0xFF)
    {}

    bool isLessFreshThan(MasterTime & other, uint8_t accuracyMs) {
        int diff = (other.tsMaster - tsMaster) - (other.tsLocal - tsLocal);
        diff = diff < 0 ? diff * -1  : diff;
        bool drifted = diff > accuracyMs;
        return  drifted && other.depth <= depth;
    }

    time_ms_t tsLocal;
    time_ms_t tsMaster;
    uint8_t  depth;
};

struct RoleCfg {
    RoleCfg(bool asMaster = false) :
        asMaster(asMaster)
    {}

    bool asMaster;
};

void serialize(ByteVector & buf, RoleCfg const & val);
void unserialize(ByteVector & buf, RoleCfg & val);

void serialize(ByteVector & buf, TimeSyncData const & val);
void unserialize(ByteVector & buf, TimeSyncData & val);

/**
 * Provides a (milli second) time synchronization service for nodes in a
 * network. It assumes that one module is the master to whose clock all
 * other nodes synchronize. It works by sending out a beacons (starting
 * at the master) governed by a trickle timer.
 *
 * It is based on the consecutive broadcast of two messages,
 * where the first message's transmission and reception are
 * timestamped by sender and receiver, respectively (at same instant).
 * The second message
 * then contains the txTimestamp to provide a reference for the receiver.
 *
 * This layer expects the timestamp information to be contained in
 * MacRxInfo metadata attached to the DataIndication. If there is
 * no such metadata available, this layer timestamps the response and
 * reception of the whole DataIndication as a reference point, which will
 * be "accurate enough" provided that no or few other short tasks are in the
 * task queue.
 *
 * Although this is less efficient than using only one message
 * which contains its own txStart timestamp, it is also
 * less demanding with regards to capabilities of the MAC layer, i.e. no
 * timestamping during send is necessary. The accuracy of both methods is
 * identical -- the instants in time used as reference are only different
 * by the propagation time which is negligible with regard to the accuracy
 * we are aiming at (~ 500 ns at 150m).
 *
 * TODO currTsData also used to store num of transmitted packets,
 *      which can be confusing
 *
 */
class TimeSyncService : public TrickleModule {
public:
    static const timeOffset_t SYNC_TIMEOUT = 100;
    static const uint8_t NOT_SYNC_DEPTH = 0xFF;
    static const timeOffset_t REFRESH_INTERVAL = 60000;
    static const timeOffset_t NUM_MISSING_MSGS_ACCEPTED = 2;
    const uint16_t NUM_REFRESH_TIMEOUTS;
    static const char *const MODULE_NAME;

    explicit TimeSyncService(const char * service_name = MODULE_NAME, uint8_t accuracyMs=0, uint16_t iMin = 50, uint8_t iMax = 12, uint8_t k = 3, bool dontResetTrickle=false);

	virtual void initialize();

	virtual void finish();

	virtual void handleTimestampMsg(DataIndication * msg);

	virtual void handleInitialMsg(DataIndication * msg);

	virtual void handleInitialResponse(DataResponse * msg);

	virtual void handleTimestampResponse(DataResponse * msg);

	bool start(RoleCfg & cfg);

	bool stop();

    void packetTimeout(Message * msg);

    void refreshTimeout(Message * msg);

    // Use NetworkTime::get() or NetworkTime::localToNetworkTime() instead
	//time_ms_t getMasterTime(time_ms_t ts);

	TimeSyncData getInfo();

	bool isSync();

	InputGate<DataIndication> gateTimestampIn;
	OutputGate<DataRequest> gateTimestampOut;

	InputGate<DataIndication> gateInitialIn;
	OutputGate<DataRequest> gateInitialOut;

private:
	virtual void transmit();

	bool isMaster();

	bool syncPending();

	Message refreshTimeoutMsg;
	Message packetTimeoutMsg;
	uint16_t refreshTimeoutCount;

	MasterTime syncData;
	bool isTransmitting;

	node_t currSyncSrc;
	TimeSyncData currTsData;

	uint8_t accuracyMs;
	bool noReset;
};


/**
 * Bridging module, which can be used on gateway nodes that act as wireless
 * extension for, e.g., a python basestation connected to a transceiver board
 * via serial connection. It simply forwards everything coming in at the
 * one gate towards the other as fast as possible. Specialty of this module is
 * that it is able to recognize TimeSyncService frames and alter the timestamp
 * of the second (timestamped) frame, so that the additional connection is made
 * transparent and the synchronization works over the two links. Additionally,
 * it uses the same timeout logic as the TimeSyncService to avoid storing
 * information for too many nodes.
 *
 * Schematic of the mechanism:
 * A: wireless network node
 * B: TimeSyncWirelessBridge-running node
 * C: serially connected basestation or local node
 *
 *    A           B          C
 *    |-----------|          |
 *    |    m1     |          |
 * ta |-----------| tb       |
 *    |           |----------|
 *    |           |    m1    |
 *    |        tb'|----------| tc
 *    |-----------|          |
 *    | [ta] m2   |          |
 *    |-----------|          |
 *    |           |----------|
 *    |           | [ts] m2  |
 *    |           |----------|
 *
 *  ts here has to be set to ts = ta + (tb' - tb) in message 2 from B to C;
 *  ta is a network timestamp, as is ts
 *  tb, tb' and tc are local timestamps of the system clock counter (palLocalTime of
 *  the corresponding node).
 *
 *  Additionally, the module does support forwarding of messages to a stack
 *  above it by using the protocolPathGateway in the constructor. This works only
 *  for messages from (and from the stack to) the secondary gates.
 *
 *  The mechanism
 *  is mainly meant to provide a possibility to receive and send RemoteAccess
 *  messages on a node without any own ID (no PAL_ID) -- this can be handled
 *  by point-to-point
 *  connections which do not really care for the destination address, but not by
 *  a mac_interface implementing MAC layer, which uses a given ID for unicast
 *  ACKs (in that sense, former attempts to build a "partial" stack, forwarding
 *  some and retrieving other messages, failed because of this addressing problem)
 */
class TimeSyncWirelessBridge: public Layer {
public:
    static const uint8_t QUEUE_LEN = 30;
    static const uint8_t PROTOCOL_PATH_MAX_LEN = 5;

public:
    TimeSyncWirelessBridge(const ByteVector& protocolPathInitial,
                           const ByteVector& protocolPathTs,
                           const ByteVector& protocolPathGateway,
                           const char * name = NULL);

    void initialize();
    virtual void handleIndication(DataIndication * msg);
    virtual void handleIndicationFromSerial(DataIndication * msg);
    virtual void handleRequest(DataRequest* msg);

    void responseWirelessToSerial(DataResponse* resp);
    void responseSerialToWireless(DataResponse* resp);

    InputGate<DataIndication> fromSerial;
    OutputGate<DataRequest> toSerial;
    InputGate<DataIndication> gateSnoopIndIn;

private:
    typedef Queue<DataIndication*, QUEUE_LEN> queue_t;
    typedef void (TimeSyncWirelessBridge::*responseHandler)(DataResponse*);

    /** to keep track of metadata associated with two time synchronization messages*/
    struct TimeSyncTransitData {
        friend class LoadableTask<TimeSyncTransitData*>;
        TimeSyncTransitData(node_t syncSrc,
                            responseHandler handler) :
            currSyncSrc(syncSrc),
            ts(0),
            pendingDepth(TimeSyncService::NOT_SYNC_DEPTH),
            handler(handler),
            justSentInitialMsg(false)
        {}


        node_t currSyncSrc;
        union {
            time_ms_t ts;
            time_ms_t delta;
        };
        uint8_t pendingDepth;
        queue_t queue;
        LoadableTask<TimeSyncTransitData*> syncTimeout;
        responseHandler handler;
        bool justSentInitialMsg;
    };

    void sendNext(TimeSyncTransitData& tstd,
                  OutputGate<DataRequest>& outputGate);

    void handle(DataIndication * msg,
            TimeSyncTransitData& tstd,
            OutputGate<DataRequest> & outputGate);

    void handleResponse(
                DataResponse* resp,
                TimeSyncTransitData& tstd,
                OutputGate<DataRequest>& outputGate);

    /** Checks if the given message is an initial message
     *  originated from a TimeSyncService module. If it is the first incoming
     *  message or one with a better (lower) depth, the the message-associated
     *  timestamp is stored for later delta-calculation and the current active
     *  synchronization is set to the message's origin node id.
     *
     *  @param[in]
     *      msg original incoming data indication with airframe attached
     *  @param[in,out]
     *      tstd information about active time synchronization process
     *  @return true, if message is an initial TimeSyncService message
     *          false, else
     */
    bool checkForInitialMessage(
            DataIndication* msg,
            TimeSyncTransitData& tstd,
            bool& discardMessage);


    bool checkForAndChangeTimestampMessage(
            DataIndication* msg,
            TimeSyncTransitData& tstd,
            bool& discardMessage);

    bool checkProtocolPathInAirframe(
            Airframe& frame,
            const ByteVector& protocolPath,
            uint8_t tmp[],
            uint8_t& i);

    void writeBackToAirframe(
            Airframe& frame,
            const uint8_t tmp[],
            const uint8_t& i);

    void resetSyncProcess(TimeSyncTransitData& tstd);

    void timeout(TimeSyncTransitData* tstd);

    /**
     * <ol>
     *   <li> storing the message-associated timestamp of a initial message from
     *        TimeSyncService, to be able to calculate
     *        the needed delta-t later. This is only done if no active synchronization
     *        is available for this direction or the incoming message has a better
     *        (lower) depth, otherwise the message is silently discarded.
     *   <li> change the timestamp in a contained TimeSyncService timestamp message
     *        if the incoming message fits the currently active time sync process.
     *        Otherwise, discard.
     *
     */


    const ByteVector& protocolPathInitial;
    const ByteVector& protocolPathTs;
    const ByteVector& protocolPathGateway;
    TimeSyncTransitData wirelessToSerial;
    TimeSyncTransitData serialToWireless;
};

} /* namespace cometos */
#endif /* ASSOCIATIONSERVICE_H_ */
