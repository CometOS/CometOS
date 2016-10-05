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

#ifndef MACABSTRACTIONLAYER_H_
#define MACABSTRACTIONLAYER_H_

#include <stdbool.h>
#include "types.h"
#include "Object.h"
#include "Airframe.h"
#include "MacAbstractionBase.h"
#include "mac_interface.h"
#include "MacStats.h"
#include "MacConfig.h"
#include "mac_constants.h"
#include "cometosError.h"
#include "PhyCount.h"

#ifndef MAC_DEFAULT_PAN
#define MAC_DEFAULT_PAN			((mac_networkId_t) MAC_NWK_BROADCAST)
#endif

namespace cometos {

void serialize(ByteVector& buf, const PhyCounts& val);
void unserialize(ByteVector& buf, PhyCounts& val);

/**
 *
 * This class maps CometOS framework to a MAC layer.
 *
 */
class MacAbstractionLayer : public MacAbstractionBase {

public:
	static MacAbstractionLayer * mal;

	MacAbstractionLayer(const char* name = NULL,
	                    const node_t* fixedAddress = NULL);

	void initialize();

	void finish();

	// Callbacks --------------------------------------------------------------

	/**
	 * @inheritDoc
	 */
	virtual void rxEnd(Airframe *frame, node_t src, node_t dst, MacRxInfo const & rxInfo);
	/**
	 * Called when packet is received, but has to be dropped due
	 * to a CRC error.
	 */
	virtual void rxDropped();

	/**
	 * Called packet transmission is finished. This callback depends
	 * on the sending mode (e.g., in case of a transmission with ack,
	 * this is called after receiving an ack).
	 *
	 *  @param result  indicating the result of the sending
	 */
	virtual void txEnd(macTxResult_t result, MacTxInfo const & txInfo);
	// COMMANDS ---------------------------------------------------------------
	/**
	 * @inheritDoc
	 */
	virtual bool sendAirframe(Airframe* frame, node_t dst, uint8_t mode = 0, const ObjectContainer* meta=NULL);

	virtual bool setNwkId(mac_networkId_t nwkId);

	virtual bool setShortAddr(node_t newAddr);

	virtual node_t getShortAddr();

	PhyCounts getPhyStats();

	void resetPhyStats();

	/**
	 *
	 */
	virtual bool listen();

	/**Enters sleep mode. Not available if in sending state or
	 * node is receiving data.
	 *
	 * @return	<code>true</code> if node enters sleeping state
	 */
	virtual bool sleep();

	/**Sets parameter for exponential backoff algorithm which is applied for
	 * TX_MODE_BACKOFF mode. Note that aUnitBackoffPeriod is fixed by the
	 * specification of the hardware.
	 */
	virtual bool configureBackoffAlgorithm(uint8_t minBE, uint8_t maxBE,
			uint8_t maxBackoffRetries);

	virtual void setPromiscuousMode(bool value);


	/**
	 * Processes an incoming rxDropped event. Called in synchronous (task) context.
	 */
	void processRxDropped(Message * msg);

	/**
	 * Processes an incoming message. Called in synchronous (task) context.
	 */
	void processRxDone(Message * msg);

	/**
	 * Processes a sendDone callback. Called in synchronous (task) context.
	 */
	void processTxDone(Message * msg);

	cometos_error_t resetStandardConfig();
	
	MacConfig getStandardConfig();

	cometos_error_t setStandardConfig(MacConfig & cfg);

	MacConfig getActiveConfig();

	/**
	 * Called directly from interrupt service routine
	 */
	void rxCallback(uint8_t * buffer,
	                uint8_t len,
	                mac_phyPacketInfo_t const * phyInfo,
	                node_t dst,
	                node_t src,
	                mac_networkId_t nwkId);

	/**
	 * Called directly from interrupt service routine
	 */
	void txDoneCallback(uint8_t const * buffer,
	                    mac_result_t result,
	                    mac_txInfo_t const * macInfo);

	/**
	 * Called directly from interrupt service routine
	 */
	void rxDroppedCallback();

	OutputGate<DataIndication> gateIndOut;
    OutputGate<DataIndication> gateSnoopIndOut;

private:

	mac_result_t changeDriverConfig(const MacConfig & cfg);

	Airframe * rxMsg;
	MacRxInfo ppi;
	node_t rxSrc;
	node_t rxDst;

	Airframe * txMsg;
	MacTxInfo mti;
	macTxResult_t txResult;
	const node_t* const fixedAddress;
	node_t address;
	mac_result_t result;
	time_ms_t sendTime;
	Message rxDroppedEvent;
	Message txEvent;
	Message rxEvent;
	MacConfig standardConfig;
	bool changeDriverConfigPending;
};

} // namespace cometos

#endif /* MACABSTRACTIONLAYER_H_ */
