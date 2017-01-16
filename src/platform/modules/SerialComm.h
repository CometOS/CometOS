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
 * @author Stefan Unterschuetz
 */

#ifndef SERIALCOMM_H_
#define SERIALCOMM_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "LowerEndpoint.h"
#include "palSerial.h"
#include "Queue.h"
#include "Callback.h"

#include "palLed.h"

namespace cometos {

#ifndef SERIAL_COMM_BAUDRATE
#define SERIAL_COMM_BAUDRATE 57600
#endif

#ifndef SERIAL_FRAME_TIMEOUT
#define SERIAL_FRAME_TIMEOUT 35
#endif

#define QUEUE_LENGTH		4

/*CLASS DECLARATION----------------------------------------------------------*/

struct SerialCommStats {
    SerialCommStats(uint32_t numReq = 0,
                    uint32_t numFail = 0,
                    uint16_t numQueueFull = 0) :
                        numReq(numReq),
                        numFail(numFail),
                        numQueueFull(numQueueFull)
    {}

    uint32_t numReq;
    uint32_t numFail;
    uint16_t numQueueFull;
};



/**Provides reliable, full-duplex serial communication. Can be used in
 * base station and sensor node.
 *
 * CRC and acknowledgements are used. CRC calculation is done in parallel
 * to reception and transmission of data in order to improve throughput.
 *
 * Note that first packet might be discarded, if sequence number is
 * already in history (last received history number is stored)
 *
 * This is a singleton object
 * Required interfaces: palSerial.h
 */
class SerialComm: public LowerEndpoint {
public:
#ifdef SERIAL_ENABLE_STATS
	SerialCommStats stats;
  	SerialCommStats getStats();

    #define SC_STATS_INIT(x) (stats.x) = 0
    #define SC_STATS_INC(x) (stats.x)++
    #define SC_STATS_RECORD(x) recordScalar((#x), (stats.x))
#else
    #define SC_STATS_INIT(x)
    #define SC_STATS_INC(x)
    #define SC_STATS_RECORD(x)
#endif

	template<typename PeripheralType>
	SerialComm(PeripheralType port,
	           const char* name = "sc",
	           uint32_t baudrate = SERIAL_COMM_BAUDRATE,
               uint16_t frameTimeout = SERIAL_FRAME_TIMEOUT) :
       		LowerEndpoint(name),
       		taskRxCallback(*this),
       		taskTx(*this),
       		taskRx(*this),
       		taskResync(*this),
       		retries(0),
       		rxTs(0),
       		txTs(0),
       		rxSeq(255),
       		txSeq(0),
       		rxCrc(0),
       		length(0),
       		baudrate(baudrate),
            frameTimeout(frameTimeout)
	{
		SC_STATS_INIT(numReq);
		SC_STATS_INIT(numFail);
		SC_STATS_INIT(numQueueFull);
		serial = PalSerial::getInstance<PeripheralType>(port);
	}

	void initialize();

	virtual void handleRequest(DataRequest* msg);

	node_t getAddr();

private:
	void tx();
	void rx();
	void resync();
	void rxCallback();
	void txHandle(DataRequest* msg);
	void rxHandle();

	void serializeMeta(DataRequest* request);

	void deserializeMeta(DataIndication* ind);

//	void responseTimeout();

	uint8_t checkParity(uint8_t b);

	void confirm(DataRequest * req,
	             bool result,
	             bool addTxInfo,
	             uint8_t retries,
	             bool isValidTxTs,
	             time_ms_t txDuration);

	template<class T, int mask>
	void serializeMetadataIfPresent(DataRequest* req, Airframe& frame, uint8_t& fb) {
	    if (req->has<T>()) {
	        LOG_DEBUG_PREFIX;
	        LOG_DEBUG_PURE("Found metadata with mask "<< (int) mask << "; AFlen=" << (int) frame.getLength());
	        frame << *(req->get<T>());
	        LOG_DEBUG_PURE(" AFlen-after=" << (int) frame.getLength() << cometos::endl);
	        fb |= mask;
	    }
	}

	template<class T, int mask>
	bool deserializeMetadataIfPresent(DataIndication* ind, Airframe& frame, uint8_t fb) {
	    T* ptr = nullptr;
        if (fb & mask) {
            LOG_DEBUG("Found " << (int) mask );
            ptr = new T();
            frame >> *ptr;
            ind->set(ptr);
            return true;
        } else {
            return false;
        }
	}


	PalSerial* serial;


	BoundedTask<SerialComm, &SerialComm::rxCallback> taskRxCallback;

	BoundedTask<SerialComm, &SerialComm::tx> taskTx;
	BoundedTask<SerialComm, &SerialComm::rx> taskRx;
	BoundedTask<SerialComm, &SerialComm::resync> taskResync;

	/**
     * Previously, the WAIT_ACK was an extra state.
     * When a new packet was arriving when an ACK was expected,
     * the state was overwritten, so the requirement of an ACK was
     * forgotten. A subsequent ACK led to a resync.
     * Now, waitingForACK is an extra flag so it can not be forgotten.
     */
    typedef enum {
	    //STATE_IDLE, STATE_WAIT_ACK, STATE_RX, STATE_FLUSH
	    STATE_IDLE, STATE_RX, STATE_FLUSH
    } state_t;

    bool waitingForACK;

    state_t state;

    /**Stores number of bytes to received as well as current state
     */
    uint8_t retries;
    AirframePtr rxBuffer;


    /** FIXME TODO instead of using a queue of requests, use a queue of
     *  dedicated datastructures which contain meta information like seq, dst, src
     *  --- this way, the airframe size can be reduced to the mac's MAC_MAX_PAYLOAD_SIZE
     */
    Queue<DataRequest*, QUEUE_LENGTH> queue;
    time_ms_t rxTs;
    time_ms_t txTs;
    time_ms_t sendTime;

    uint8_t rxSeq;
    uint8_t txSeq;

    uint16_t rxCrc;
    uint8_t crcPkt[2];
    uint8_t length;  // remaining length of the curr rx packet
    uint32_t baudrate;
    uint16_t frameTimeout;
};

void serialize(ByteVector & buf, const SerialCommStats & val);
void unserialize(ByteVector & buf, SerialCommStats & val);

}

#endif /* SERIALCOMM_H_ */
