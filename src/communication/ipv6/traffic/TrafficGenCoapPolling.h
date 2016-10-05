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

#ifndef MULTITESTCOAP_H_
#define MULTITESTCOAP_H_

#include "TrafficGenSim.h"
#include "TrafficGenListener.h"
#include "CoAPLayer.h"
#include "Statistics.h"
#include "palLocalTime.h"

namespace cometos_v6 {

/**
 * A CoAP Resource with the simple functionality to let the TrafficGen traffic
 * generator send a UDP packet.
 */
class TrafficGenStartSending : public CoAPResource {
public:
    TrafficGenStartSending();
    virtual ~TrafficGenStartSending();

    virtual void handleRequest(cometos_v6::CoAPMessage* request);
    virtual void handleDuplicateRequest(cometos_v6::CoAPMessage* request, uint8_t type);

    virtual char* getRepresentation();
    virtual void setRepresentation(const char* repr, uint16_t len);

    void setTrafficGen(TrafficGen * mt);
private:
    TrafficGen* mt;
    char state[2];
};


/**
 * Depends on the TrafficGen traffic generator. Can be used to initiate data
 * transfer at nodes defined by a target list. Simply iterates through the
 * list and sends a CoAP message to all nodes, commanding them to send a
 * UDPDatagram in turn. The number of iterations is configurable.
 *
 * Is an ABC for subclasses, which have to implement methods to iterate
 * through the list of nodes to poll for data.
 */
class TrafficGenCoapPolling : public cometos::Module, public CoAPListener, public TrafficGenListener {
public:

    typedef SumsMinMax<timeOffset_t, uint16_t, uint32_t, uint64_t> collectionDurationType;

    static const char * RESOURCE_PATH;
    static const char * const MODULE_NAME;
    static const uint8_t SEQ_NUM_LEN;

    TrafficGenCoapPolling(const char * service_name = MODULE_NAME);
    virtual ~TrafficGenCoapPolling();

    virtual void initialize();

    virtual void finish();

    /**
     * Called by TrafficGen on reception of a UDP datagram
     */
    virtual void handleIncomming(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length);

    /**
     * Called by CoAP on reception of a CoAP response
     */
    virtual void incommingCoAPHandler(const CoAPMessage* message, const bool state);


     const collectionDurationType& getCollectionDurations();

private:

    /**
     * Read or create the node list to use, must be overwritten by subclass
     */
    virtual IPv6Address initializeTargetList() = 0;

    /**
     * Get the next target node.
     *
     * @param[out] addr contains a pointer to the next target in the list
     *                  or NULL, if the list is empty
     * @return true, the next target would have beed the end of the list
     *               (instead, the first element is returned)
     *         false else
     */
    virtual bool getNextTarget(IPv6Address * addr) = 0;

    void nextTarget();

    void timeout(cometos::Message * msg);

    void pollNext(cometos::Message * msg);

    uint16_t numCollections;
    uint16_t currCollection;

    IPv6Address currNode;

    CoAPLayer* coap;
    TrafficGenStartSending mtssResource;

    time_ms_t startTime;

    collectionDurationType collectionDurations;

    uint16_t numTimeouts;

    uint16_t timeoutInterval;

    cometos::Message timeoutMsg;
};

void wordToByteArray(uint16_t x, char a[]);
uint16_t byteArrayToWord(const uint8_t a[]);

} /* namespace cometos */

#endif /* MULTITESTCOAP_H_ */
