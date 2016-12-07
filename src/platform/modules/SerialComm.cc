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

#include "SerialComm.h"
#include "TaskScheduler.h"
#include "Airframe.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "crc16.h"
#include "MacAbstractionBase.h"
#include "palLed.h"
#include "OverwriteAddrData.h"
#include "palLocalTime.h"
#include "MacControl.h"
#include "ForwardMacMeta.h"
#include <stdint.h>

#include "palId.h"

namespace cometos {

#define MAX_LENGTH			AIRFRAME_MAX_SIZE

#define ACK_OFFSET			0x7E

#if (ACK_OFFSET <= MAX_LENGTH)
#error "ACK_OFFSET <= MAX_LENGTH"
#endif

#if (AIRFRAME_MAX_SIZE > MAX_LENGTH)
#error "airframe size is insufficient"
#endif

//#define ACK_PENDING_BIT		1
#define ACK_SUCCESS_BIT		1

#define CRC_LEN             2

#define MAX_RETRIES			5

#define FLAG_MAC_CONTROL_MASK           0x01
#define FLAG_MAC_CONTROL_SHIFT          0

#define FLAG_MAC_RXINFO_MASK            0x02
#define FLAG_MAC_RXINFO_SHIFT           1

#define FLAG_REQ_SERIAL_RESPONSE_MASK   0x04
#define FLAG_REQ_SERIAL_RESPONSE_SHIFT  2

#define FLAG_SERIAL_RESPONSE_MASK       0x08
#define FLAG_SERIAL_RESPONSE_SHIFT      3

#define FLAG_MAC_TXINFO_MASK            0x10
#define FLAG_MAC_TXINFO_SHIFT           4


#define TASK_RESPONSE_TIMEOUT 250

uint8_t SerialComm::checkParity(uint8_t b) {
	uint8_t c = 0;

	while (b) {
		if (b & 1) {
			c++;
		}
		b >>= 1;
	}

	if (c % 2) {
		return 1;
	} else {
		return 0;
	}
}

void SerialComm::rxCallback() {
	getScheduler().replace(taskRx);
}

void SerialComm::initialize() {
	// initialize UART driver
	rxBuffer = new Airframe();
	serial->init(baudrate, &taskRxCallback, NULL, NULL);
	state = STATE_IDLE;
	retries = 0;
    waitingForACK = false;
    LOG_DEBUG("Init palSerial; baudrate=" << baudrate);
#ifdef SERIAL_ENABLE_STATS
	remoteDeclare(&SerialComm::getStats, "gs");
#endif
}

#ifdef SERIAL_ENABLE_STATS
SerialCommStats SerialComm::getStats() {
    return stats;
}
#endif


void SerialComm::handleRequest(DataRequest* msg) {
    SC_STATS_INC(numReq);
	txHandle(msg);
}

node_t SerialComm::getAddr() {
#ifdef PAL_ID
	return palId_id();
#else
	return MAC_BROADCAST;
#endif
}


void SerialComm::serializeMeta(DataRequest* request) {
    uint8_t flagByte = 0;
    Airframe& frame = request->getAirframe();

    serializeMetadataIfPresent<MacControl, FLAG_MAC_CONTROL_MASK>(request, frame, flagByte);
    serializeMetadataIfPresent<MacRxInfo, FLAG_MAC_RXINFO_MASK>(request, frame, flagByte);
    serializeMetadataIfPresent<MacTxInfo, FLAG_MAC_TXINFO_MASK>(request, frame, flagByte);
    serializeMetadataIfPresent<RequestResponseOverSerial, FLAG_REQ_SERIAL_RESPONSE_MASK>(request, frame, flagByte);
    serializeMetadataIfPresent<SerialResponse, FLAG_SERIAL_RESPONSE_MASK>(request, frame, flagByte);

    // mark which kind of metadata is attached
    frame << flagByte;
    LOG_DEBUG("Serializing metadata done, flag="
            << cometos::hex << (int) flagByte << cometos::dec
            << " len=" << (int) frame.getLength());
}

void SerialComm::deserializeMeta(DataIndication* ind) {
    Airframe& frame = ind->getAirframe();
    // retrieve flag byte, which indicates meta information object's presence
    uint8_t flagByte;
    frame >> flagByte;
    LOG_DEBUG("Start deserializing; flag=" << (int) flagByte);
    // mind that order of deserialization matters here (confer serializeMeta)!!!!!
    deserializeMetadataIfPresent<SerialResponse, FLAG_SERIAL_RESPONSE_MASK>(ind, frame, flagByte);

    deserializeMetadataIfPresent<RequestResponseOverSerial, FLAG_REQ_SERIAL_RESPONSE_MASK>(ind, frame, flagByte);

    deserializeMetadataIfPresent<MacTxInfo, FLAG_MAC_TXINFO_MASK>(ind, frame, flagByte);
    if (ind->has<MacTxInfo>()) {
        ind->get<MacTxInfo>()->tsInfo.isValid = false;
    }

    if (!deserializeMetadataIfPresent<MacRxInfo, FLAG_MAC_RXINFO_MASK>(ind, frame, flagByte)) {
        ind->set(new MacRxInfo(LQI_MAX, true, MacRxInfo::RSSI_EMULATED, true, rxTs));
    }

    deserializeMetadataIfPresent<MacControl, FLAG_MAC_CONTROL_MASK>(ind, frame, flagByte);

}


void SerialComm::txHandle(DataRequest* request) {
    if (queue.full()) {
#ifdef SERIAL_ENABLE_STATS
	    stats.numQueueFull++;
#endif
	    LOG_DEBUG("");
		confirm(request, false, false, 0, false, 0);
	} else {

	    LOG_DEBUG("Next msg: len=" << (int) request->getAirframe().getLength()
	               << "|dst=" << request->dst);
	    serializeMeta(request);

		// attach src and destination addr
		if (request->has<OverwriteAddrData>()) {
            OverwriteAddrData * meta = request->get<OverwriteAddrData>();
            request->getAirframe() << meta->dst << meta->src;
//          getCout() << "S:d=" << meta->dst << "|s=" << meta->src << " -- ";
        } else {
            request->getAirframe() << request->dst
                    << getAddr();
//          getCout() << "S:d=" << request->dst << "|s=" << getAddr() << " -- ";
        }

		// attach sequence number
		request->getAirframe() << (txSeq++);

//		request->getAirframe().printFrame(&getCout());
		queue.push(request);
		getScheduler().replace(taskTx);
	}
}
void SerialComm::rxHandle() {
	// remove CRC and ...
    ASSERT(rxBuffer!=NULL);
	ASSERT(rxBuffer->getLength()>=0);

	// retrieve sequence number
	uint8_t seq;
    node_t src;
    node_t dst;
	(*rxBuffer) >> seq  >> src >> dst;

    if (seq == rxSeq) {
        // filter duplicate
        return;
    }

    rxSeq = seq;

    DataIndication *ind = new DataIndication(rxBuffer, src, dst);

    deserializeMeta(ind);

//	getCout() << "R:s=" << src << "|d=" << dst << " -- ";
//	ind->getAirframe().printFrame(&getCout());

	LowerEndpoint::sendIndication(ind);

	rxBuffer = new Airframe;
}

// MAIN TASKS------------------------------------------------------------------

void SerialComm::confirm(
        DataRequest * req,
        bool result,
        bool addTxInfo,
		uint8_t retries,
		bool isValidTxTs,
		time_ms_t txDuration) {
	DataResponse * resp = new DataResponse(result ? DataResponseStatus::SUCCESS : DataResponseStatus::FAIL_UNKNOWN);
	mac_dbm_t rssi;
	if (addTxInfo) {
		if (req->dst == MAC_BROADCAST || !result) {
			rssi = RSSI_INVALID;
		} else {
			rssi = MacRxInfo::RSSI_EMULATED;
		}
//		getCout() << "tsValid=" << isValidTxTs << "|reportedTs=" << txTs << endl;
		MacTxInfo * info = new MacTxInfo(req->dst, retries, 0,
				rssi, rssi, txDuration, isValidTxTs, txTs);
		resp->set(info);
//		getCout() << "tsData.tsValid=" << info->tsInfo.isValid << "|tsData.ts=" << info->tsInfo.ts << endl;
	}
	LOG_DEBUG("success=" << (int) result << "|retries=" << (int) retries << "|txDuration=" << txDuration);
	req->response(resp);
	delete(req);
}

void SerialComm::tx() {
	if (state != STATE_IDLE || waitingForACK) {
		return;
	}

	if (MAX_RETRIES < retries) {
		ASSERT(!queue.empty());
#ifdef SERIAL_ENABLE_STATS
		stats.numFail++;
#endif
		LOG_DEBUG("no ack");
		confirm(queue.front(), false, true, retries, false, palLocalTime_get() - txTs);
		retries = 0;
		queue.pop();
	}

	if (!queue.empty()) {
		uint8_t len = queue.front()->getAirframe().getLength() + 2;
		uint8_t *data = queue.front()->getAirframe().getData();
		uint16_t crc = 0xffff;

		uint8_t header = len;
		if (checkParity(header)) {
			header = header | 0x80;
		}
		//	printf("%d %x\n",len,len);

		// Start sending of data, in parallel ...
		serial->write(&header, 1);

		// here we assume that the palSerial will send out that first
		// byte "immediately enough" to get a valid timestamp
		if (retries == 0) {
		    txTs = palLocalTime_get();
		}

		serial->write(data, len - 2);

		// we calculate CRC checksum and...
		for (uint8_t i = 0; i < len - 2; i++) {
			crc = crc16_update(crc, data[i]);
		}

		// transmit CRC
		uint8_t upper, lower;
		upper = 0xFF & (crc >> 8);
		lower = 0xFF & crc;
		serial->write(&lower, 1);
		serial->write(&upper, 1);

		retries++;
		// set frame timeout for the case that no ack is received
		//printf("Data Sent\n");
		getScheduler().replace(taskResync, frameTimeout);
//		printf("SCHEDULE timeout %u\n", palLocalTime_get());
		//state = STATE_WAIT_ACK;
        waitingForACK = true;
		getScheduler().replace(taskRx);
	}
}

void SerialComm::rx() {
//	printf("ENTER RX %u\n", palLocalTime_get());

	if (state == STATE_FLUSH) {
		return;
	}

	if (0 == length) {
		uint8_t b;
		if (!serial->read(&b, 1)) {
//            printf("RETURN %u\n", palLocalTime_get());
			return;
		}

		// timestamp reception of first byte
		// TODO could be moved to interrupt itself for more precision
		rxTs = palLocalTime_get();

		if (checkParity(b)) {
		    LOG_DEBUG("Parity check fail");
			getScheduler().replace(taskResync, frameTimeout);
			state = STATE_FLUSH;
			return;
		}

		b = b & 0x7F;
		if (waitingForACK && b >= ACK_OFFSET) {
		    LOG_DEBUG("rcvd ACK");
		    time_ms_t txDuration = palLocalTime_get() - txTs;
			state = STATE_IDLE;
            waitingForACK = false;
			getScheduler().remove(taskResync);
//            printf("remove\n");

			if (b & ACK_SUCCESS_BIT) {
			    LOG_DEBUG("ACK success");
//			    printf("got Ack %x %u\n",b, palLocalTime_get());
				ASSERT(!queue.empty());
				DataRequest* originalReq = queue.front();
//				if (originalReq->has<RequestResponseOverSerial>()) {
//				    awaitResponse = true;
//				    LOG_DEBUG("wait for response until " << palLocalTime_get() + TASK_RESPONSE_TIMEOUT);
//				    getScheduler().replace(taskResponseTimeout, TASK_RESPONSE_TIMEOUT);
//				    //schedule
//				} else {
				    getScheduler().replace(taskTx);
				    LOG_DEBUG("tx success");
                    confirm(originalReq, true, true, retries, true, txDuration);
                    retries = 0;
                    queue.pop();
//				}
			} else {
			    LOG_DEBUG("ACK no success");
			    // reschedule to do next retry
			    getScheduler().replace(taskTx);
//			    printf("Ack invalid %u\n", palLocalTime_get());
			}

            /* The ACK_PENDING_BIT is timing critical, but not necessary for the correct functionality, so remove it.
             * 1. A->B Message
             * 2. B->A Message
             * 3. B->A ACK for 1., containing PENDING_BIT, since 2. was not acked
             * 4. A->B ACK for 2.
             * 5. B receives 3. and falsely thinks there is a transmission pending.
			if (b & ACK_PENDING_BIT) {
                pushDebug('P');
				state = STATE_RX;
				getScheduler().replace(taskResync, frameTimeout);
			}
            */
			getScheduler().replace(taskRx);
			return;
		} else if (b <= (MAX_LENGTH + 2) && b > 2) {
			length = b;
			state = STATE_RX;
			getScheduler().replace(taskResync, frameTimeout);
//			printf("length %d %u\n",length, palLocalTime_get());
			rxBuffer->setLength(length - 2);
			rxCrc = 0xffff;

		} else {
			getScheduler().replace(taskResync, frameTimeout);
			state = STATE_FLUSH;
			return;
		}
	}

	// try to receive packet
	if (length > 0) {
		uint8_t *data = NULL;
		uint8_t received = 0;

		if (length > 2) {
			data = rxBuffer->getData() + (rxBuffer->getLength() - (length - 2));
			received = serial->read(data, length - 2);
		} else {
			data = &crcPkt[2 - length];
			received = serial->read(data, length);
		}

		length -= received;

		for (uint8_t i = 0; i < received; i++) {
			rxCrc = crc16_update(rxCrc, data[i]);
		}

		// total frame is received, now ---
		if (0 == length) {
			getScheduler().remove(taskResync);

            // The frame reception resync can be canceled, 
            // but add a new one if the ACK is still pending
            if(waitingForACK) {
		        getScheduler().replace(taskResync, frameTimeout);
            }

			uint8_t ack = ACK_OFFSET;

			// check whether CRC is correct
			if (rxCrc == 0) {
				ack |= ACK_SUCCESS_BIT;
			}
            /*
			if (!queue.empty()) {
				ack |= ACK_PENDING_BIT;
			}
            */
			if (checkParity(ack)) {
				ack |= 0x80;
			}
			LOG_DEBUG("Sending ACK: " << (int) ack);
			serial->write(&ack, 1);

			if (rxCrc == 0) {
				rxHandle();
			}

			state = STATE_IDLE;
			tx(); // why not schedule this?
		}
		getScheduler().replace(taskRx);
	}

}

//void SerialComm::responseTimeout() {
//    ASSERT(awaitResponse);
//    DataRequest* origReq = queue.front();
//    queue.pop();
//    LOG_DEBUG("");
//    confirm(origReq, false, false, 0, false, 0);
//}

void SerialComm::resync() {
//    palLed_toggle(1);
#if defined(BOARD_local) || defined(BOARD_python)
    //printf("FLUSHING %u\n", palLocalTime_get());
#endif
    LOG_DEBUG("FLUSHING");
	while (serial->read(rxBuffer->getData(), MAX_LENGTH) > 0) {
	}
    length = 0;
	state = STATE_IDLE;
    waitingForACK = false;
	if (!queue.empty()) {
		getScheduler().replace(taskTx, intrand(frameTimeout)+1);
	}
}


void serialize(ByteVector & buf, const SerialCommStats & val) {
    serialize(buf, val.numReq);
    serialize(buf, val.numFail);
    serialize(buf, val.numQueueFull);
}
void unserialize(ByteVector & buf, SerialCommStats & val) {
    unserialize(buf, val.numQueueFull);
    unserialize(buf, val.numFail);
    unserialize(buf, val.numReq);
}

}
