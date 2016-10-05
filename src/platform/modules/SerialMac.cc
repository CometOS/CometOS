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

#include "SerialMac.h"
#include "MacControl.h"
#include "ForwardMacMeta.h"
#include "MacAbstractionBase.h"
#include "palLocalTime.h"
#include "palId.h"

namespace cometos {

Define_Module(SerialMac);

const uint8_t SerialMac::QUEUE_SIZE;
const timeOffset_t SerialMac::SERIAL_RESPONSE_TIMEOUT;

SerialMac::SerialMac(const char * name) :
		Layer(name),
		snoopIndOut(this, "snoopIndOut"),
		originalResponseTarget(NULL),
		responseTimeoutTask(CALLBACK_MET(&SerialMac::responseTimeout, *this)),
		seq(0),
        currReqId(NULL),
		promiscuousMode(false)
{
}

void SerialMac::finish() {
}


void SerialMac::initialize() {
	Layer::initialize();

}

void SerialMac::handleRequest(DataRequest* msg) {
    if (!reqQueue.full()) {
        LOG_DEBUG("Handle Request for " << msg->dst << " queueSize=" << (int) reqQueue.getSize());
        reqQueue.push(msg);
        if (reqQueue.getSize() == 1) {
            sendNext();
        }
    } else {
        DataResponse* resp = new DataResponse(false);
        msg->response(resp);
        delete msg;
        LOG_ERROR("Queue full");
    }
}


void SerialMac::handleIndication(DataIndication* msg) {
    LOG_DEBUG("dst=" << msg->dst << "|src=" << msg->src);
    if (msg->has<SerialResponse>()) {
        SerialResponse* respFromSc = msg->get<SerialResponse>();
        LOG_DEBUG("SC ind-resp; success=" << (int) respFromSc->success
                   << "|seq=" << (int) respFromSc->seq);
        if (respFromSc->seq == seq) {
            if (reqQueue.empty()) {
                // if the queue is empty here, we already must have had a 
                // timeout and do NOT send a response, because that has
                // already been done
                // if another message arrived in the meantime, we would
                // not get here, because a new seq number is used in that case
            } else {
                DataRequest* originalReq = reqQueue.front();
                DataResponse* resp = new DataResponse(respFromSc->success);

                resp->set(msg->unset<MacTxInfo>());
                originalReq->response(resp);
                delete(originalReq);
                reqQueue.pop();
            }
            getScheduler().remove(responseTimeoutTask);
            delete(msg);
            sendNext();
        } else {
            // got late SerialResponse and new tx frame has arrived already
            LOG_DEBUG("Seq not matching (got: " << (int) respFromSc->seq << " expect: " << (int) seq << "), discard");
            delete(msg);
        }
    } else {
        LOG_DEBUG("No ind-resp, forward to upper");
        if (msg->dst == palId_id() || msg->dst == MAC_BROADCAST) {
            LOG_DEBUG("gateIndOut");
            gateIndOut.send(msg);
        } else {
            if (promiscuousMode) {
                LOG_DEBUG("snoopIndOut");
                snoopIndOut.send(msg);
            } else {
                LOG_DEBUG("promisc. mode = false");
            }
        }
    }
}

void SerialMac::handleResponse(DataResponse* resp) {
    if (resp->success) {
        if (currReqId == resp->getRequestId()) {
            // we get a response to the still active request, 
            // delete response, start timer and wait for response in indication
            delete(resp);
            getScheduler().replace(responseTimeoutTask, SERIAL_RESPONSE_TIMEOUT);
            LOG_DEBUG("Sc resp successful, schedule timeout at " << palLocalTime_get() + SERIAL_RESPONSE_TIMEOUT);
        } else {
            // we get a response to a request we already have received the 
            // SerialResponse for (and therefore changed the currReqId, which
            // means, we have already responded to the original request
            // --> ignore the response
            delete(resp);
        }
    // SerialComm reports a failure      
    } else {
        LOG_DEBUG("Sc resp unsuccessful, check: " << (uintptr_t) currReqId << " resp=" << (uintptr_t) resp->getRequestId());
        if (currReqId == resp->getRequestId()) {
             
            if (!reqQueue.empty()) {
                // remove reqId from the response we forward to the upper layer
                resp->setResponseId(NULL);
                currReqId = NULL;
                DataRequest* originalReq = reqQueue.front();
                originalReq->response(resp);
                delete originalReq;
                reqQueue.pop();
                sendNext();
            } else {
                // queue empty, but we already have received 
                // a SerialResponse successfully (but not the ACK
                // somehow); do not answer again
                delete resp;
            }
        } else {
            // we already received a SerialResponse, but not an ACK,
            // and have the next frame in the queue already sent
            // delete response belonging to old request and do not
            // respond again
            delete resp;
        }
    }
}

void SerialMac::responseTimeout() {
    LOG_INFO("response timeout");
    if (reqQueue.empty()) { 
        // msg already removed, ignore timeout
    } else {
        DataRequest* originalReq = reqQueue.front();
        originalReq->response(new DataResponse(false));
        delete originalReq;
        reqQueue.pop();
    }
    sendNext();
}

void SerialMac::setPromiscuousMode(bool promiscuousMode) {
    this->promiscuousMode = promiscuousMode;
}

bool SerialMac::getPromiscuousMode() {
    return promiscuousMode;
}

void SerialMac::sendNext() {
    if (reqQueue.empty()) {
        LOG_DEBUG("empty queue");
        return;
    }

    DataRequest* originalReq = reqQueue.front();

    currReqId = new RequestId();
    DataRequest* req = new DataRequest(originalReq->dst,
                                       originalReq->decapsulateAirframe(),
                                       createCallback(&SerialMac::handleResponse),
                                       currReqId);
    ObjectContainer* oc = req;

    // TODO this messes around with the not sufficiently well implemented
    // copy constructors / assignment operators or Message/Request<T>/DataRequest
    // we use the only sensibly available copy constructor of ObjectContainer to
    // get the metadata;
    // instead of moving the metadata, we copy it here... *urgh*
    oc->ObjectContainer::operator=(*originalReq);
    bool hasMacControl = originalReq->has<MacControl>();
    seq = (seq+1) & (127);
    req->set(new RequestResponseOverSerial(seq));
    ASSERT(hasMacControl == req->has<MacControl>());

    LOG_DEBUG("Send next; seq=" << (int) seq
               << "|hasMacControl=" << hasMacControl
               << "|");
    gateReqOut.send(req);
}

}
