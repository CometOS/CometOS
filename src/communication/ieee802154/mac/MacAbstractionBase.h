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

#ifndef MACABSTRACTIONINTERFACE_H_
#define MACABSTRACTIONINTERFACE_H_

#include "LongScheduleModule.h"
#include "mac_definitions.h"
#include "MacMetaRetries.h"

#define MAC_MODULE_NAME "mac"

struct DestinationNwk : public cometos::Object {
    DestinationNwk(mac_networkId_t dstNwk = MAC_NWK_BROADCAST) :
        dstNwk(dstNwk)
    {}

    virtual  ~DestinationNwk() {
    }

    mac_networkId_t dstNwk;

    cometos::Object* getCopy() const {
        return new DestinationNwk(this->dstNwk);
    }
};

namespace cometos {



struct TimeSyncInfo {
    TimeSyncInfo(bool isValid = false,
                 time_ms_t ts = 0) :
        isValid(isValid),
        ts(ts)
    {}

    bool isValid;
    time_ms_t ts;

};

/**
 * Used by the MAC layer to provide information on retries
 * to the upper layer
 */
class MacTxInfo : public Object {
public:
    MacTxInfo(node_t destination = 0,
                uint8_t numRetries = 0,
                uint8_t numCCARetries = 0,
                uint8_t remoteRssi = RSSI_INVALID,
                uint8_t ackRssi = RSSI_INVALID,
                time_ms_t txDuration = 0,
                bool isValidTxTs = false,
                time_ms_t txTs = 0) :
        Object(),
        destination(destination),
        numRetries(numRetries),
        numCCARetries(numCCARetries),
        remoteRssi(remoteRssi),
        ackRssi(ackRssi),
        txDuration(txDuration),
        tsInfo(isValidTxTs, txTs)
    {}

    virtual Object* getCopy() const {
        return new MacTxInfo(this->destination,
                             this->numRetries,
                             this->remoteRssi,
                             this->ackRssi,
                             this->txDuration,
                             this->tsInfo.isValid,
                             this->tsInfo.ts);
    }

    node_t destination;
    uint8_t numRetries;
    uint8_t numCCARetries;
    mac_dbm_t remoteRssi;
    mac_dbm_t ackRssi;
    time_ms_t txDuration;
    TimeSyncInfo tsInfo;
};

class MacRxInfo: public Object {
public:
    static const mac_dbm_t RSSI_EMULATED = -1;

    MacRxInfo(lqi_t lqi=0,
              bool lqiValid = false,
              mac_dbm_t rssi = RSSI_INVALID,
              bool tsValid = false,
              time_ms_t ts = 0) :
            Object(), lqi(lqi), lqiIsValid(lqiValid), rssi(rssi), tsInfo(tsValid, ts) {
    }

    virtual Object* getCopy() const {
        return new MacRxInfo(lqi, lqiIsValid, rssi, tsInfo.isValid, tsInfo.ts);
    }
    lqi_t lqi;
    bool lqiIsValid;
    mac_dbm_t rssi;
    TimeSyncInfo tsInfo;
};

void serialize(ByteVector& buf, const TimeSyncInfo& val);
void unserialize(ByteVector& buf, TimeSyncInfo& val);

/** Does NOT serialize TimeSyncInfo, because it does not make
 * sense to transport local time stamps to another node. */
void serialize(ByteVector& buf, const MacTxInfo& val);
void unserialize(ByteVector& buf, MacTxInfo& val);

/** Does NOT serialize TimeSyncInfo, because it does not make
 * sense to transport local time stamps to another node. */
void serialize(ByteVector& buf, const MacRxInfo& val);
void unserialize(ByteVector& buf, MacRxInfo& val);


/**
 * Generic interface of the MAC abstraction layer. Should be extended by
 * platform specific MAC abstraction layers.
 *
 * TODO Unify concept for constant definition -- there are constants defined
 *      redundantly between this header and the platform-specific
 *      mac_interface.h
 */
class MacAbstractionBase : public LongScheduleModule {
public:

    MacAbstractionBase(const char * name = NULL) :
        LongScheduleModule(name)
    {}

    // Callbacks --------------------------------------------------------------

    /**
     * Called when an over the air packet is received.
     *
     * @param   pkt     the received packet
     * @param   src     the source address of the packet
     * @param   dst     the destination address of the packet
     *
     */
    virtual void rxEnd(Airframe *frame, node_t src, node_t dst, MacRxInfo const & info) = 0;
    /**
     * Called when packet is received, but has to be dropped due
     * to a CRC error.
     */
    virtual void rxDropped() = 0;

    /**
     * Called packet transmission is finished. This callback depends
     * on the sending mode (e.g., in case of a transmission with ack,
     * this is called after receiving an ack).
     *
     *  @param result      indicating the result of the sending
     */
    virtual void txEnd(macTxResult_t result, MacTxInfo const & info) = 0;


    // Commands ---------------------------------------------------------------

    /**
     * Send packet with selected sending mode. Iff this function returns
     * true the txEnd function will be invoked.
     * Intended receiver needs to be in the same mode operation.
     *
     * If a DestinationNwk object is attached to the Airframe,
     * this module will send the message out with the corresponding nwkId
     * set to the contained value.
     *
     * @param frame  actual data send over the air
     * @param dst    ID of the destination node
     * @param mode   mode of operation (ACKs, CCA, Backoffs), receiver must
     *               at least use the same mode for ACKs for transmission to
     *               be successful
     */
    virtual bool sendAirframe(Airframe* frame, node_t dst, uint8_t mode = 0, const ObjectContainer* meta=NULL) = 0;


    /**
     * Set the network ID of this MAL.
     * @param nwkId new network ID
     * @return true, if the network id was set
     *         false, if the request could not be processed
     */
    virtual bool setNwkId(mac_networkId_t nwkId) = 0;

    virtual bool setShortAddr(node_t newAddr) = 0;

    virtual node_t getShortAddr() = 0;

    /**
     *
     */
    virtual bool listen() = 0;

    /**Enters sleep mode. Not available if in sending state or
     * node is receiving data.
     *
     * @return  <code>true</code> if node enters sleeping state
     */
    virtual bool sleep() = 0;


    /**
     * Sets parameter for exponential backoff algorithm which is applied for
     * TX_MODE_BACKOFF mode. Note that aUnitBackoffPeriod is fixed by the
     * specification of the hardware. The actual backoff time for a given exponent
     * can be retrieved via getBackoffTime.
     *
     * @param minBE             minimum backoff exponent
     * @param maxBE             maximum backoff exponent
     * @param maxBackoffRetries maximum number of consecutive backoff retries
     */
    virtual bool configureBackoffAlgorithm(uint8_t minBE, uint8_t maxBE,
                uint8_t maxBackoffRetries) = 0;


    /**
     * Set or unset promiscuous mode.
     *
     * @param value  if true, MAL enters promiscuous mode
     *               if false, MAL leaves promiscuous mode
     */
    virtual void setPromiscuousMode(bool value) = 0;

};

}
#endif /* MACABSTRACTIONINTERFACE_H_ */
