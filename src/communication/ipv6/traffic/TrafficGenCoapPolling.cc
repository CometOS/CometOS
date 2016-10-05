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

#include "TrafficGenCoapPolling.h"
#include "IPv6Address.h"
#include "CoAPLayer.h"
#include "NeighborDiscovery.h"
#include "palLocalTime.h"


namespace cometos_v6 {

const char * TrafficGenCoapPolling::RESOURCE_PATH = "sendData";
const char * const TrafficGenCoapPolling::MODULE_NAME = "tgcp";
const uint8_t TrafficGenCoapPolling::SEQ_NUM_LEN = 2;

TrafficGenStartSending::TrafficGenStartSending() :
    CoAPResource(),
    mt(NULL)
{
    state[0] = '0';
    state[1] = 0;
}

TrafficGenStartSending::~TrafficGenStartSending() {

}

void TrafficGenStartSending::setTrafficGen(TrafficGen * mt) {
    this->mt = mt;
}

void TrafficGenStartSending::handleRequest(cometos_v6::CoAPMessage* request) {
    cometos_v6::CoAPMessage* answer = makeMessage();
    cometos_v6::URL url(cometos_v6::URL::COAP, request->getAddr(),
            request->getMessageSrcPort());

    uint8_t methodCode;
    uint8_t incomingMsgCode = request->getMessageCode();


    if(incomingMsgCode == COAP_POST) {
        LOG_DEBUG("POST received; len=" << (uint16_t) request->getMessagePayloadLen() << ": " << cometos::endl << request->getMessagePayload());

        if (request->getMessagePayloadLen() == TrafficGenCoapPolling::SEQ_NUM_LEN) {
            LOG_DEBUG("POST ")
            // If this request is a POST request
            methodCode = COAP_CHANGED;
        } else {
            // length invalid
            methodCode = COAP_BAD_REQUEST;
        }
    } else if (incomingMsgCode == COAP_GET) {
        methodCode = cometos_v6::COAP_CONTENT;
    } else {
        //If a method is used, that is not supported
        methodCode = COAP_NOTIMPLEMENTED;
    }

    initMessage(answer,
                url,
                COAP_PORT,
                COAP_ACK,
                methodCode,
                request->getMessageID(),
                request->getMessageToken(),
                NULL, 0,
                getRepresentation(),
                strlen(getRepresentation()),
                false,
                false);
    sendMessage(answer);

    uint16_t seq = byteArrayToWord(request->getMessagePayload());

    mt->setSeqNumAndSendMessage(seq);
}

void TrafficGenStartSending::handleDuplicateRequest(cometos_v6::CoAPMessage* request, uint8_t type) {
    // WOOT?
}

char* TrafficGenStartSending::getRepresentation() {
    return state;
}

void TrafficGenStartSending::setRepresentation(const char* repr, uint16_t len) {
    ASSERT(len == 1);
    state[0] = *repr;
}


TrafficGenCoapPolling::TrafficGenCoapPolling(const char * service_name) :
    Module(service_name),
    numCollections(0),
    currCollection(0),
    coap(NULL),
    startTime(0),
    numTimeouts(0),
    timeoutInterval(0)
{
}

TrafficGenCoapPolling::~TrafficGenCoapPolling() {
}

void TrafficGenCoapPolling::initialize() {
    TrafficGen * mt = (TrafficGen *) getModule("tg");
    coap = (CoAPLayer *) getModule(CoAPLayer::MODULE_NAME);
    ASSERT(mt != NULL);
    ASSERT(coap != NULL);

    coap->bindPort(this, cometos_v6::COAP_PORT);
    bool isMaster = false;

    CONFIG_NED(isMaster);
    CONFIG_NED(numCollections);
    CONFIG_NED(timeoutInterval);

    mtssResource.setTrafficGen(mt);
    if(isMaster) {
        mt->subscribeToIncommingDatagramEvent(this);
        currNode = initializeTargetList();
        if (currNode != IPv6Address(0,0,0,0,0,0,0,0)) {
            schedule(new cometos::Message(), &TrafficGenCoapPolling::pollNext, 0);
        }
        startTime = palLocalTime_get();
    } else {
        // configure CoAPResource and add it to CoAP layer
        mtssResource.setRessourceData(&RESOURCE_PATH, 1,
                   COAP_RES_R | COAP_RES_W, COAP_PORT, NULL);
        mtssResource.setRepresentation("0", 1);

        coap->insertResource(&mtssResource);
    }
}

void TrafficGenCoapPolling::finish() {
    RECORD_SCALAR(numTimeouts);
}


void TrafficGenCoapPolling::handleIncomming(IPv6Address const & src,
                                           uint16_t srcPort,
                                           uint16_t dstPort,
                                           const uint8_t * data,
                                           uint16_t length) {
    ENTER_METHOD_SILENT();
    uint16_t seq = ((uint16_t)(data[0] << 8) | data[1]);
    LOG_DEBUG("Received IP datagram: " << src.str()
                        << " seq: " << seq
                        << " srcPort: " << srcPort
                        << " dstPort: " << dstPort
                        << " dataLen: " << length);
    if ((src == currNode) && (seq == currCollection)) {
        cancel(&timeoutMsg);
        nextTarget();
    } else {
        LOG_DEBUG("Expected " << currNode.str() << "|" << currCollection << "; got " << src.str() << "|" << seq);
        ASSERT(false);
    }
}

void TrafficGenCoapPolling::incommingCoAPHandler(const CoAPMessage* message, const bool state) {
    ENTER_METHOD_SILENT();
    LOG_DEBUG("Received CoAP response from " << message->getAddr().str()
                    << " state: " << (uint16_t) state);
}

void TrafficGenCoapPolling::nextTarget() {
    ENTER_METHOD_SILENT();
#ifdef ENABLE_LOGGING
    IPv6Address lastNode = currNode;
#endif
    bool listFinished = getNextTarget(&currNode);
    if (listFinished) {
        currCollection++;
        collectionDurations.add(palLocalTime_get() - startTime);
        LOG_INFO("Duration for all Nodes: " << palLocalTime_get() - startTime);
        startTime = palLocalTime_get();
    } else {
        // nothing to be done, we already advanced to the next target, just
        // check if we have to continue polling
    }

    if (currCollection < numCollections) {
        schedule(new cometos::Message(), &TrafficGenCoapPolling::pollNext,0);
    } else {
        // we are finished with all collections, do nothing
    }
    LOG_DEBUG("Switched to next target: " << lastNode.str() << "-->" << currNode.str());
}

void TrafficGenCoapPolling::timeout(cometos::Message * msg) {
    LOG_DEBUG("timed out, continuing with next target");
    nextTarget();
}

void TrafficGenCoapPolling::pollNext(cometos::Message * msg) {
    ENTER_METHOD_SILENT();
    delete(msg);
    LOG_DEBUG("SEND sendData request to " << currNode.str());
    cometos_v6::URL url(cometos_v6::URL::COAP, currNode);

    char a[SEQ_NUM_LEN];
    wordToByteArray(currCollection, a);


    url.addURIPart(RESOURCE_PATH, strlen(RESOURCE_PATH), URIPart::URIPATH);
    int8_t error = coap->sendMessage(url,
                            COAP_PORT,
                            cometos_v6::COAP_CON,
                            cometos_v6::COAP_POST,
                            NULL, 0,
                            a, SEQ_NUM_LEN,
                            false,
                            false,
                            1);
    (void) error; // remove unused warning
    LOG_DEBUG("SEND msg; error=" << (uint16_t) error << "|expectedSeq=" << currCollection);
    schedule(&timeoutMsg, &TrafficGenCoapPolling::timeout, timeoutInterval);
}

const TrafficGenCoapPolling::collectionDurationType & TrafficGenCoapPolling::getCollectionDurations() {
    return collectionDurations;
}

void wordToByteArray(uint16_t x, char a[]) {
    a[0] = x >> 8;
    a[1] = x & 0xFF;
}

uint16_t byteArrayToWord(const uint8_t a[]) {
    return (a[0] << 8) + a[1];
}

} /* namespace cometos */
