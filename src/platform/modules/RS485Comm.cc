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

#include "RS485Comm.h"
#include "TaskScheduler.h"
#include "Airframe.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "crc16.h"
#include "MacAbstractionBase.h"
#include "palLed.h"
#include "OverwriteAddrData.h"
#include <stdint.h>
#include "RS485CmdAnnotation.h"

#include "palId.h"

using namespace cometos;

#define MAX_LENGTH			AIRFRAME_MAX_SIZE
#define RS485_POLL_INTERVAL   1000
#define POLLFWDCMDNUMBER 192
#define POLLTYPE 0
#define FWDTYPE 1

const char * const RS485Comm::MODULE_NAME = "rsc";

void RS485Comm::initialize() {
    if(rxBufferRS485 == NULL) {
        rxBufferRS485 = new cometos::Airframe();
    }

    if(rxBufferSendUp == NULL) {
        rxBufferSendUp = new cometos::Airframe();
    }

	if(master) {
		rs485Master->init(masterAddress);
		initRx();
	}
	else {
		rs485Slave->init(slaveAddress,CALLBACK_MET(&RS485Comm::dataPending,*this),interruptPin);
		cometos_error_t ret = rs485Slave->depositReceptionBuffer(rxBufferRS485->getData(), rxBufferRS485->getMaxLength(), CALLBACK_MET(&RS485Comm::rxFinishedCallback,*this));
		ASSERT(ret == COMETOS_SUCCESS);
	}
}

void RS485Comm::restart() {
    if(!master) {
        rs485Slave->restart();
    }
}

// RECEPTION ------------------------------------------------------------------

// Master: Gets called from pin interrupt or if pin is detected to be low otherwise
void RS485Comm::initRx() {
	//    cometos::getCout() << "RX" << cometos::endl;
	if(master) {
		if(interruptPin->get() == 0) {
			rs485Master->receive(slaveAddress, rxBufferRS485->getData(), rxBufferRS485->getMaxLength(), POLLFWDCMDNUMBER, CALLBACK_MET(&RS485Comm::rxFinishedCallback,*this));
			// return value is checked in the callback
		}
		cometos::getScheduler().replace(taskRx, RS485_POLL_INTERVAL);
	}
}

bool RS485Comm::dataPending() {
	return false;
}

// Gets called from the RS485 as soon as the data was received from interrupt context
void RS485Comm::rxFinishedCallback(uint8_t *rxBuf, uint8_t len, uint8_t cmdNumber, uint8_t typeOrStatus, cometos_error_t result) {
	if(result == COMETOS_SUCCESS && len > 0) {
		rxBufferRS485->setLength(len);

		// is there a fresh buffer in rxBufferSendUp?
		if(rxBufferSendUp->getLength() == 0) {
			// swap pointers to prepare RS485 buffer for next reception
			cometos::Airframe* tmp = rxBufferSendUp;
			rxBufferSendUp = rxBufferRS485;
			rxBufferRS485 = tmp;

			RS485CmdAnnotation* cmd = new RS485CmdAnnotation(cmdNumber,typeOrStatus);
			rxBufferSendUp->set(cmd);
			cometos::getScheduler().replace(taskSendUp);
		}
		// otherwise sendUpTask is still pending,
		// so drop packet and reuse old buffer

		if(!master) {
			cometos_error_t ret = rs485Slave->depositReceptionBuffer(rxBufferRS485->getData(), rxBufferRS485->getMaxLength(), CALLBACK_MET(&RS485Comm::rxFinishedCallback,*this));
			ASSERT(ret == COMETOS_SUCCESS);
		}

	}

	// check if still more packets are to be received
	if(master && interruptPin->get() == 0) {
		cometos::getScheduler().replace(RS485Comm::taskRx);
	}
}

// Task that is scheduled by rxFinishedCallback
void RS485Comm::sendUpTask() {
	ASSERT(rxBufferSendUp!=NULL);

	//cometos::getCout() << "<<<<<< RX <<<<<<<" << cometos::endl;
	//cometos::getCout() << "sendUp " << (uint32_t)rxBufferSendUp->getLength() << " Bytes" << cometos::endl;
	//rxBufferSendUp->print();

	cometos::DataIndication *ind = new cometos::DataIndication(rxBufferSendUp, palId_id(), 0 /* not to be interpreted by upper layer */);
	cometos::MacRxInfo * phy = new cometos::MacRxInfo(LQI_MAX,
			true,
			cometos::MacRxInfo::RSSI_EMULATED,
			true,
			0);

	ind->set(phy);

	//ind->getAirframe().print();

	LowerEndpoint::sendIndication(ind);

	rxBufferSendUp = new cometos::Airframe();
	rxBufferSendUp->setLength(0);
}


// TRANSMISSION ------------------------------------------------------------------

// Is called from upper layer to issue a transmission 
void RS485Comm::handleRequest(cometos::DataRequest* msg) {
	//cometos::getCout() << ">>>>> TX >>>>>>" << cometos::endl;
	//cometos::getCout() << "handleRequest of length " << (uint32_t)msg->getAirframe().getLength() << cometos::endl;
	//msg->getAirframe().print();

	if (queue.full()) {
		confirm(msg, false, false, false);
		delete msg;
	} else {
		queue.push(msg);
		cometos::getScheduler().replace(taskTx);
	}
}

// Task that is scheduled by handleRequest
void RS485Comm::tx() {
	//cometos::getCout() << "tx()" << cometos::endl;
	if (!queue.empty() && !txPending) {
		txPending = true;

		uint8_t len = queue.front()->getAirframe().getLength();
		uint8_t *data = queue.front()->getAirframe().getData();

		//cometos::getCout() << "TX with length " << (uint32_t)len << cometos::endl;
		//queue.front()->getAirframe().print();

		cometos_error_t ret = COMETOS_ERROR_FAIL;
		if(master) {
			uint8_t cmdNumber = POLLFWDCMDNUMBER;
			uint8_t type = FWDTYPE;

			if(queue.front()->getAirframe().has<RS485CmdAnnotation>()) {
				RS485CmdAnnotation* cmd = queue.front()->getAirframe().get<RS485CmdAnnotation>();
				cmdNumber = cmd->cmd;
				type = cmd->typeOrStatus;
			}

			ret = rs485Master->transmit(slaveAddress, data, len, CALLBACK_MET(&RS485Comm::txFinishedCallback,*this), cmdNumber, type);
		}
		else {
            uint8_t cmdNumber = 0xFF;
            uint8_t status = 0xFF;

            if(queue.front()->getAirframe().has<RS485CmdAnnotation>()) {
                RS485CmdAnnotation* cmd = queue.front()->getAirframe().get<RS485CmdAnnotation>();
                cmdNumber = cmd->cmd;
                status = cmd->typeOrStatus;
            }

			ret = rs485Slave->depositTransmissionData(data, len, CALLBACK_MET(&RS485Comm::txFinishedCallback,*this), cmdNumber, status);
		}

		if(ret != COMETOS_SUCCESS) {
			txPending = false;
		}
	}
}

// Gets called from the RS485 as soon as the data was transmitted from interrupt context
void RS485Comm::txFinishedCallback(cometos_error_t result) {
	ASSERT(!queue.empty());

	confirm(RS485Comm::queue.front(), true, true, true);
	delete RS485Comm::queue.front();
	queue.pop();
	txPending = false;

	//cometos::getCout() << "<<< TX <<<" << cometos::endl;

	// check if packets are to be received
	if(master && interruptPin->get() == 0) {
		cometos::getScheduler().replace(taskRx);
	}

	// check if still more packets are to be transmitted
	if(!queue.empty()) {
		cometos::getScheduler().replace(taskTx);
	}
}

// HELPER ------------------------------------------------------------------

void RS485Comm::confirm(cometos::DataRequest * req, bool result, bool addTxInfo,
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

