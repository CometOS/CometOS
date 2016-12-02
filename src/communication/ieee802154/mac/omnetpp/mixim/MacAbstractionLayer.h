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
 * @author Stefan Unterschütz
 */

#ifndef MACABSTRACTIONLAYER_H_
#define MACABSTRACTIONLAYER_H_

#include <stdbool.h>
#include "types.h"
#include "LowerEndpoint.h"
#include "Object.h"
#include "Airframe.h"
#include "FSM.h"
#include "BaseMacLayer.h"
#include "MacAbstractionBase.h"
#include "LqiProvider.h"
#include "DeciderResultEmpiric802154.h"
#include "MacHeader.h"
#include "logging.h"

namespace cometos {

#define	MAC_TYPE_DATA  0
#define MAC_TYPE_ACK  1

/**
 * Stores id of a node
 */
class NodeId: public Object {
public:
    NodeId(node_t value) :
            value(value) {
    }
    virtual ~NodeId() {}

    Object* getCopy() const {
        return new NodeId(this->value);
    }
    node_t value;
};


class MacEvent : public FSMEvent {
public:
    enum : uint8_t {
        SEND_REQUEST = FSMEvent::USER_SIGNAL_START,
        TIMEOUT_CCA,
        TIMEOUT_BACKOFF,
        TIMEOUT_ACK,
        TIMEOUT_SIFS,
        FRAME_TRANSMITTED,
        ACK_RECEIVED,
        FRAME_RECEIVED,
        ACKED_FRAME_RECEIVED,
        EXPECTED_FRAME_DROPPED,
        RADIO_SWITCHING_OVER,
        TX_DONE
    };

    MacEvent(uint8_t signal)
    : FSMEvent(signal) {
    }

    MacEvent()
    : FSMEvent() {
    }
};

/**
 * This class provides a MAC abstraction layer implementation
 * which maps the CometOS framework to a MiXiM PHY layer.
 */
class MacAbstractionLayer: public MacAbstractionBase, public FSM<MacAbstractionLayer, MacEvent> {
public:
	MacAbstractionLayer(const char * name = NULL,
	                    const node_t* const address = NULL);

	virtual ~MacAbstractionLayer();


	/**************************************************************************
	 ** General MacAbstractionInterface  **************************************
	 *************************************************************************/

	// Callbacks --------------------------------------------------------------

	/**
	 * @inheritDoc
	 */
	virtual void rxEnd(Airframe * frame, node_t src, node_t dst, MacRxInfo const & info);

	/**
	 * @inheritDoc
	 */
	virtual void rxDropped();

	/**
	 * @inheritDoc
	 */
	virtual void txEnd(macTxResult_t result, MacTxInfo const & info);


	// Commands ---------------------------------------------------------------

	/**
	 * @inheritDoc
	 */
	virtual bool sendAirframe(Airframe* frame, node_t dst, uint8_t mode = 0, const ObjectContainer* meta=NULL);


	/**
	 * @inheritDoc
	 */
	virtual bool setNwkId(mac_networkId_t id);

	/**
	 * @inheritDoc
	 */
	virtual bool listen();

	/**
	 * @inheritDoc
	 */
	virtual bool sleep();

	/**
	 * @inheritDoc
	 */
	virtual bool configureBackoffAlgorithm(uint8_t minBE, uint8_t maxBE,
			uint8_t maxFrameRetries);


	/**
	 * @inheritDoc
	 */
	void setPromiscuousMode(bool value);

	/***/
	uint16_t getBackoffTime(uint8_t be);


	inline bool setShortAddr(node_t newAddr)  {
		return true;
	}

	virtual node_t getShortAddr();

	void initialize();

	virtual void finish();

	void setLqiProvider(LqiProvider * provider);

	/**
	 */
	// CONFIGURATION ----------------------------------------------------------

    void MACWorks(LongScheduleMsg * msg) {
        LOG_INFO("MACWorks");
        failure = false;
        delete msg;
    }
    void MACFailed(LongScheduleMsg * msg) {
        LOG_INFO("MACFailed");
        failure = true;
        delete msg;
    }

protected:
    struct FrameStats {
        FrameStats() :
            droppedWithInterference(0),
            droppedWithoutInterference(0),
            failTxAlready(0),
            failTxDuringRx(0),
            successWithInterference(0),
            successWithoutInterference(0)
        {}

        void reset() {
            droppedWithInterference = 0;
            droppedWithoutInterference = 0;
            failTxAlready = 0;
            failTxDuringRx = 0;
            successWithInterference = 0;
            successWithoutInterference =0;
        }

        uint32_t droppedWithInterference;
        uint32_t droppedWithoutInterference;
        uint32_t failTxAlready;
        uint32_t failTxDuringRx;
        uint32_t successWithInterference;
        uint32_t successWithoutInterference;
    };



	// stats for debugging
	uint32_t sendingSucceed;
	uint32_t sendingFailed;
	uint32_t txDuringRx;
	uint32_t rxDuringTx;
	uint32_t received;

	// the following structures keep count of successful and dropped data
	// and ack frames with and without interference
	FrameStats ackFrames;
    FrameStats dataFrames;

	// OLD INTERFACE

	virtual void handleMessage(omnetpp::cMessage *msg);
	virtual void receiveLowerControl(omnetpp::cMessage *msg);
	virtual void receiveLowerData(omnetpp::cMessage *msg);
	virtual void timeout(Message *msg);



private:
	void handleAckFromLower(mac_dbm_t rssi,
                  const MacHeader& header,
                  const DeciderResultEmpiric802154 * dre);

	void handleDataFromLower(
	              const MacHeader & header,
	              Airframe *pkt,
	              const DeciderResultEmpiric802154 * dre,
	              lqi_t lqi,
                  bool lqiValid,
                  mac_dbm_t rssi);

	void handleRxDrop(const char * frameType,
                      node_t dst,
                      node_t src,
                      const DeciderResultEmpiric802154* dre,
                      FrameStats & stats);

	void logRxFrame(const char * frameType,
                    node_t dst,
                    node_t src,
                    const DeciderResultEmpiric802154* dre,
                    FrameStats & stats);

	void createFailureResumeEvents(omnetpp::cXMLElementList& list,
	        const TypedDelegate<LongScheduleMsg> & handler);

	/**
     * Send packet with selected sending mode to a given destination
     * and a given network. Iff this function returns
     * true the txEnd function will be invoked.
     * Intended receiver needs to be in the same mode operation.
     *
     * @param frame  actual data send over the air
     * @param dst    ID of the destination node
     * @param dstNwk ID of the destination network
     * @param mode   mode of operation (ACKs, CCA, Backoffs), receiver must
     *               at least use the same mode for ACKs for transmission to
     *               be successful
     */
    virtual bool sendToNetwork(Airframe* frame, node_t dst, mac_networkId_t dstNwk, uint8_t mode = 0);

	typedef FSM<MacAbstractionLayer, MacEvent> fsm_t;

	fsmReturnStatus stateInit(MacEvent& e);
	fsmReturnStatus stateSleep(MacEvent&);
	fsmReturnStatus stateIdle(MacEvent&);
	fsmReturnStatus stateBackoff(MacEvent&);
	fsmReturnStatus stateCCA(MacEvent&);
	fsmReturnStatus stateTransmitFrame(MacEvent&);
	fsmReturnStatus stateWaitAck(MacEvent&);
	fsmReturnStatus stateWaitSIFS(MacEvent&);
	fsmReturnStatus stateTransmitAck(MacEvent&);

	fsmReturnStatus sendRequest();
	void scheduleBackoff();

	/**
	 * Allows the receive of packets not intended for this node.
	 */
	//	bool promiscuousMode;
	//	uint8_t sequence;
	//	uint8_t ackTimeout;
	void attachSignal(omnetpp::cPacket* mac, simtime_t startTime, double power);
	Signal* createSignal(simtime_t start, simtime_t length, double power,
			double bitrate);
	Mapping* createRectangleMapping(simtime_t start, simtime_t end,
			double value);
	Mapping
	* createConstantMapping(simtime_t start, simtime_t end, double value);


	// MAC SENDING CONFIGURATION
	/** The bit rate at which we transmit in bits per second*/
	double bitrate;

protected:
	/**The power (in mW) to transmit with.*/
	double txPower;
	double currTxPower;

    bool isChannelBusy();

private:
	double maxTxPower;
	double minTxPower;

	/**CCA detection time */
	double ccaDetectionTime;

	/** CCA RSSI Threshold */
	double ccaThreshold;

	bool macResetCcaBe;

	/** maximum number of backoffs before frame drop */
	unsigned int macMaxCSMABackoffs;
	/** base time unit for calculating backoff durations */
	double aUnitBackoffPeriod;

protected:
	bool txWaitsForBusyRx;

    double aTurnaroundTime;

private:
	/** 802.15.4 modes:
     *  0 (3a): Busy, if energy is above threshold OR compliant signal detected (1 OR 2)
     *  1: Busy, if energy is above a threshold
     *  2: Busy, if a 802.15.4 compliant signal with same PHY characteristics is detected
     *  3 (3b): Busy, if energy is above threshold AND compliant signal is detected (1 AND 2) */
	int macCcaMode;

	/**
	 * brief Minimum backoff exponent.
	 * Only used for exponential backoff method.
	 */
	int macMinBE;
	/**
	 * brief Maximum backoff exponent.
	 * Only used for exponential backoff method.
	 */
	int macMaxBE;

	/**Maximum time between a packet and its ACK
	 *
	 * Usually this is slightly more than the tx-rx turnaround time
	 * The channel should stay clear within this period of time.
	 */
	double sifs;

	/**The amount of time the MAC waits for the ACK of a packet.*/
	double macAckWaitDuration;

	/** maximum number of frame retransmissions without ack */
	unsigned int macMaxFrameRetries;

	/** accually used number of retransmission, changeable by metadata */
    unsigned int currMaxFrameRetries;

	macTxResult_t lastResult;

	/// current network id ("PAN ID")
	mac_networkId_t nwkId;

	// temporary values
	uint8_t remoteRssi;
	mac_dbm_t ackRssi;


protected:
    /** Reset retry and backoff configuration to initial values,
     * e.g., after the reception of a packet interrupted a transmission
     */
    void resetTxParams();

    // MAC VARIABLES
    uint8_t txMode; // current tx mode
    uint8_t txSeq; // current sequence number of send packet
    Airframe* txPkt;
    int currMinBe; // current backoff exponent

    // store state for failing MAC layer
    bool suppressRxWhileTxPending;
    bool enable;

protected:

	LqiProvider * lqiProvider;

private:

	bool failure;
	/**The bit length of the ACK packet.*/
	//	int ackLength; // TODO TO IMPLEMENT
	//	 // current packet for transmission
	void doBackoff();
	void backoffDone();
	void ccaDone(bool result);
	void sendDone();

	void scheduleTxDone(macTxResult_t result);

	virtual void txDone(macTxResult_t result);
	void ackRecv(bool result);

	void scheduleTimer(double time, uint8_t kind, const char* name);

	unsigned int nb;
	unsigned int accBackoffs;
	unsigned int retryTx;

	time_ms_t sendTime;

	Message* timer;

	int lowerGateIn;
	int lowerGateOut;
	int lowerControlIn;
	int lowerControlOut;

	bool promiscuousMode;

protected:

    /** Handler to the physical layer.*/
    MacToPhyInterface* phy;

protected:
    OutputGate<DataIndication>  gateIndOut;
    OutputGate<DataIndication> gateSnoopIndOut;
};

}

#endif /* MACABSTRACTIONLAYER_H_ */
