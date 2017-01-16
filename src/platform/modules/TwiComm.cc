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

#include "TwiComm.h"
#include "TaskScheduler.h"
#include "Airframe.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "crc16.h"
#include "MacAbstractionBase.h"
#include "palLed.h"
#include "OverwriteAddrData.h"
#include <stdint.h>

#include "palId.h"

using namespace cometos;

#define MAX_LENGTH			AIRFRAME_MAX_SIZE
#define TWI_POLL_INTERVAL   1000

TwiComm* TwiComm::instance = NULL;

const char * const TwiComm::MODULE_NAME = "twc";

void TwiComm::initialize() {
	ASSERT(instance == NULL); // TODO explicit singleton
	instance = this;
	rxBufferTwi = cometos::make_checked<cometos::Airframe>();
	rxBufferSendUp = cometos::make_checked<cometos::Airframe>();

	if(master) {
		twiMaster->init();
        initRx();
	}
	else {
		twiSlave->init(address);
		cometos_error_t ret = twiSlave->depositReceptionBuffer(rxBufferTwi->getData(), rxBufferTwi->getMaxLength(), TwiComm::rxFinishedCallback);
        ASSERT(ret == COMETOS_SUCCESS);
	}
}

// RECEPTION ------------------------------------------------------------------

// Master: Gets called from pin interrupt or if pin is detected to be low otherwise
void TwiComm::initRx() {
//    cometos::getCout() << "RX" << cometos::endl;
    if(instance->master) {
        if(instance->interruptPin->get() == 0) {
		    cometos_error_t ret = twiMaster->receive(address, rxBufferTwi->getData(), rxBufferTwi->getMaxLength(), true, TwiComm::rxFinishedCallback);
            ASSERT(ret == COMETOS_SUCCESS);
        }
        cometos::getScheduler().replace(instance->taskRx, TWI_POLL_INTERVAL);
	}
}

// Gets called from the TWI as soon as the data was received from interrupt context
void TwiComm::rxFinishedCallback(uint8_t *rxBuf, uint8_t len, cometos_error_t result) {
	if(result == COMETOS_SUCCESS && len > 0) {
		instance->rxBufferTwi->setLength(len);

        // is there a fresh buffer in rxBufferSendUp?
        if(instance->rxBufferSendUp->getLength() == 0) {
            // swap pointers to prepare TWI buffer for next reception
            cometos::AirframePtr tmp = instance->rxBufferSendUp;
            instance->rxBufferSendUp = instance->rxBufferTwi;
            instance->rxBufferTwi = tmp;

		    cometos::getScheduler().add(instance->taskSendUp);
        }
        // otherwise sendUpTask is still pending,
        // so drop packet and reuse old buffer

        if(!instance->master) {
		    cometos_error_t ret = instance->twiSlave->depositReceptionBuffer(instance->rxBufferTwi->getData(), instance->rxBufferTwi->getMaxLength(), TwiComm::rxFinishedCallback);
            ASSERT(ret == COMETOS_SUCCESS);
        }
        
	}

    // check if still more packets are to be received
    if(instance->master && instance->interruptPin->get() == 0) {
        cometos::getScheduler().replace(TwiComm::instance->taskRx);
    }
}

// Task that is scheduled by rxFinishedCallback
void TwiComm::sendUpTask() {
	ASSERT(rxBufferSendUp);
	ASSERT(rxBufferSendUp->getLength()>=sizeof(node_t)*2);

	//cometos::getCout() << "sendUp " << (int)rxBufferSendUp->getLength() << " Bytes" << cometos::endl;
    //cometos::getCout() << "<<<<<< RX <<<<<<<" << palLocalTime_get() << cometos::endl;

	node_t src;
	node_t dst;
	(*rxBufferSendUp) >> src >> dst;
	cometos::DataIndication *ind = new cometos::DataIndication(rxBufferSendUp, src, dst);
	cometos::MacRxInfo * phy = new cometos::MacRxInfo(LQI_MAX,
	                                                  true,
	                                                  cometos::MacRxInfo::RSSI_EMULATED,
	                                                  true,
	                                                  0);

	ind->set(phy);

	LowerEndpoint::sendIndication(ind);

	rxBufferSendUp = cometos::make_checked<cometos::Airframe>();
    rxBufferSendUp->setLength(0);
}


// TRANSMISSION ------------------------------------------------------------------

// Is called from upper layer to issue a transmission 
void TwiComm::handleRequest(cometos::DataRequest* msg) {
    //cometos::getCout() << ">>>>> TX >>>>>>" << palLocalTime_get() << cometos::endl;
    //cometos::getCout() << "handleRequest of length " << (int)msg->getAirframe().getLength() << cometos::endl;
    //msg->getAirframe().print();

	if (queue.full()) {
		confirm(msg, false, false, false);
		delete msg;
	} else {
		if (msg->has<OverwriteAddrData>()) {
			OverwriteAddrData * meta = msg->get<OverwriteAddrData>();
			msg->getAirframe() << meta->dst << meta->src;
		} else {
			msg->getAirframe() << msg->dst
					<< palId_id();
		}
		queue.push(msg);
		cometos::getScheduler().add(taskTx);
	}
}

// Task that is scheduled by handleRequest
void TwiComm::tx() {
	if (!queue.empty() && !txPending) {
		txPending = true;

		uint8_t len = queue.front()->getAirframe().getLength();
		uint8_t *data = queue.front()->getAirframe().getData();

		//cometos::getCout() << "TX with length " << (int)len << cometos::endl;
        //queue.front()->getAirframe().print();

		cometos_error_t ret = COMETOS_ERROR_FAIL;
		if(master) {
			ret = twiMaster->transmit(address, data, len, TwiComm::txFinishedCallback);
		}
		else {
			ret = twiSlave->depositTransmissionData(data, len, true, TwiComm::txFinishedCallback); 
            interruptPin->clear();
		}

		if(ret != COMETOS_SUCCESS) {
			txPending = false;
		}
	}
}

// Gets called from the TWI as soon as the data was transmitted from interrupt context
void TwiComm::txFinishedCallback(cometos_error_t result) {
	ASSERT(!instance->queue.empty());

    if(!instance->master) {
        instance->interruptPin->set();
    }

	instance->confirm(TwiComm::instance->queue.front(), true, true, true);
	delete TwiComm::instance->queue.front();
	instance->queue.pop();
	instance->txPending = false;

    //cometos::getCout() << "<<< TX <<<" << cometos::endl;

    // check if packets are to be received
    // TODO sometimes an interrupt gets lost so the poll routine has to catch it, why?
    if(instance->master && instance->interruptPin->get() == 0) {
        cometos::getScheduler().replace(instance->taskRx);
    }

    // check if still more packets are to be transmitted
    if(!instance->queue.empty()) {
        cometos::getScheduler().add(instance->taskTx);
    }
}

// HELPER ------------------------------------------------------------------

void TwiComm::confirm(cometos::DataRequest * req, bool result, bool addTxInfo,
		bool isValidTxTs) {
	cometos::DataResponse * resp = new cometos::DataResponse(result ? DataResponseStatus::SUCCESS : DataResponseStatus::FAIL_UNKNOWN);
	mac_dbm_t rssi;
	if (addTxInfo) {
		if (req->dst == MAC_BROADCAST || !result) {
			rssi = RSSI_INVALID;
		} else {
			rssi = cometos::MacRxInfo::RSSI_EMULATED;
		}
		cometos::MacTxInfo * info = new cometos::MacTxInfo(req->dst, 0, 0,
				rssi, rssi, isValidTxTs, 0);
		resp->set(info);
	}
	req->response(resp);
}

