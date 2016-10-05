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
 * @author: Marvin Burmeister, Martin Ringwelski
 */

#ifndef COAP_LAYER_H_
#define COAP_LAYER_H_

#include "cometos.h"
#include "UDPLayer.h"
#include "CoAPMessage.h"
#include "URL.h"
#include "ReceivedMessages.h"
#include "SentMessages.h"
#include "ResourceList.h"

#include "coapconfig.h"

namespace cometos_v6 {

const int8_t ASYNC_NEEDED = -1;

class CoAPListener {
public:
    virtual void incommingCoAPHandler(
            const CoAPMessage* message, const bool state) = 0;

    virtual ~CoAPListener() {};
};

struct CoAPCallback_t {
        uint16_t            port;
        CoAPListener*       handler;
        CoAPCallback_t(): port(0), handler(NULL) {}
};

class CoAPLayer: public cometos::Module, public UDPListener {
        friend class CoAPMessage;
public:
    static const char *const MODULE_NAME;
    static const char* RESOURCE_DISCOVERY_PATH;
    static const uint16_t RETRANSMIT_INTERVAL;

    CoAPLayer(const char * service_name = NULL);
    ~CoAPLayer() {}

    virtual void initialize();
    void finish();

    uint16_t bindPort(CoAPListener* listener, uint16_t port = 0)
    {

        LOG_INFO("Bind Port ");

        if(port == 0) {
            port = COAP_PORT;
        }

        if (udp == NULL) {
        	initialize();
        }
        ASSERT(udp != NULL);

        port = udp->bind(this, port);      //Try to bind

        if(port == 0) {                    //If port was occupied or something else happened
            port = udp->bind(this, 0);     //try binding with random port
        }

        if(port == 0) {
            return 0;
        }

        LOG_DEBUG(port);

        CoAPCallback_t* respListener = getNextFreeListener(responseListeners);
        if (respListener != NULL) {
            LOG_DEBUG("Set Listener to Port " << port);
            respListener->handler = listener;
            respListener->port = port;
        }
        else{
            //If no free listeners available, unbind udp port and return 0
            udp->unbindPort(port);
            port = 0;
        }

        return port;
    }
    bool unbindPort(uint16_t port);

/*    CoAPMessage* prepareMessage(const URL& url, uint16_t srcPort,
            uint8_t messageType, uint8_t messageCode,
            CoAPMessageOption** options, uint8_t optionsLen, const char* payload,
            uint16_t payloadLen, bool handleOptions, bool handlePayload,
            uint8_t* errCode);
*/
    CoAPMessageOption* prepareOption(uint16_t optNr, const char* optData,
            uint16_t optLen, uint8_t optOrder){
        CoAPMessageOption* ret = new CoAPMessageOption(&buffer, optNr, optData, optLen, optOrder);
        if (ret->isValid()) {
            return ret;
        }
        delete ret;
        return NULL;
    }

    uint8_t sendMessage(CoAPMessage* message, bool handleData);
    uint8_t sendMessage(const URL& url, uint16_t srcPort, uint8_t messageType,
            uint8_t messageCode, CoAPMessageOption** options,
            uint8_t optionsLen, const char* payload, uint16_t payloadLen,
            bool handleOptions, bool handlePayload, uint8_t tag);

    CoAPMessage* prepareMessage() {

        CoAPMessage* newMessage = new CoAPMessage(&buffer);
        return newMessage;
    }

    void initMessage(CoAPMessage* msg, const URL& url, uint16_t srcPort,
            uint8_t messageType, uint8_t messageCode, uint16_t messageID,
            Token token, CoAPMessageOption** options, uint8_t optionsLen,
            const char* payload, uint16_t payloadLen, bool handleOptions,
            bool handlePayload);


    bool insertResource(CoAPResource* insertMe) {
//    	udp->bind(this, insertMe->getLocalPort());
        return resources.insertResource(insertMe, this, &buffer);
    }

    bool deleteResource(CoAPResource* deleteMe) {
        return resources.deleteResource(deleteMe);
    }

private:
    bool isResourceDiscovery(CoAPMessage* request);

    void retransmitTimerFired(cometos::Message* msg);

    void simulateDelayedAnswer(cometos::LongScheduleMsg* msg);

    CoAPCallback_t* getNextFreeListener(CoAPCallback_t Listeners[COAP_MAX_LISTENERS]) {
        //LOG_DEBUG("Listeners: " << (uintptr_t)Listeners);
        for (uint8_t i = 0; i < COAP_MAX_LISTENERS; i++) {
            if (Listeners[i].port == 0) {
                LOG_DEBUG("i = " << (int)i);
                return &(Listeners[i]);
            }
        }
        return NULL;
    }

    CoAPCallback_t* getCorrespondingListener(CoAPCallback_t Listeners[COAP_MAX_LISTENERS], uint16_t port);

    virtual void udpPacketReceived(const IPv6Address& src,
               uint16_t srcPort,
               uint16_t dstPort,
               const uint8_t* data,
               uint16_t length);

    void sendResetMessage(CoAPMessage* recMessage, uint16_t port){
        sendResponse(recMessage, port,
                COAP_RST,
                COAP_EMPTY,
                recMessage->getMessageID(),
                Token(),
                NULL, 0,
                NULL, 0,
                false, false);
    }
    void sendAckMessage(CoAPMessage* recMessage, uint16_t port){
        //Only an ACK, no piggieback answer!
        uint16_t messageID = recMessage->getMessageID();

        sendResponse(recMessage, port,
                COAP_ACK,
                COAP_EMPTY,
                messageID,
                Token(),
                NULL, 0,
                NULL, 0,
                false, false);

    }
    void sendPiggybackMessage(CoAPMessage* recMessage, uint16_t port,
            const char* payload, uint16_t payloadLen) {
        //An ACK, that already contains the response
        uint16_t messageID = recMessage->getMessageID();
        Token token = recMessage->getMessageToken();

        sendResponse(recMessage, port,
                COAP_ACK,
                COAP_CONTENT,
                messageID,
                token,
                NULL, 0,
                payload, payloadLen,
                false, false);

    }
    int sendConfirmableMessage(CoAPMessage* recMessage, uint16_t port);

    uint16_t getNextMessageID(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort);
    Token getNextToken(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, uint8_t length);

    CoAPBuffer_t* getCoAPBuffer(){
        return &buffer;
    }

    void deleteAllMessages();

    void handleResponse(CoAPMessage* message, const IPv6Address& addr,
            uint16_t srcPort, uint16_t dstPort, bool state,
            bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token());


    void sendResponse(CoAPMessage* recMessage, uint16_t port,
            uint8_t messageType, uint8_t messageCode,
            uint16_t messageID, Token token, CoAPMessageOption** options,
            uint8_t optionsLen, const char* payload, uint16_t payloadLen,
            bool handleOptions, bool handlePayload) {
        URL url(URL::COAP, recMessage->getAddr(), recMessage->getMessageSrcPort());

        CoAPMessage* query = prepareMessage();

        query->init(url,
                port,
                messageType,
                messageCode,
                messageID,
                token,
                options, optionsLen,
                payload, payloadLen,
                handleOptions, handlePayload);  //initiate message
        sendMessage(query, false);    //and try to send it
    }

    cometos_v6::UDPLayer*   udp;
    ReceivedMessages<COAP_MAX_MESSAGE_BUFFER>   rcvdMessages;
    SentMessages<COAP_MAX_MESSAGE_BUFFER>       sentMessages;


    cometos::Message timer;

    uint16_t currMessageID;

    LowpanBuffer<COAP_SET_BUFFER_SIZE, COAP_SET_BUFFER_ENTRIES> buffer;

    //listens for responses. Critical for clients.
    CoAPCallback_t responseListeners[COAP_MAX_LISTENERS];

    ResourceList<COAP_MAX_RESOURCES>    resources;

    int counter;            //let only every 3rd message be processed

    uint8_t maxTokenLen;    //Determines how long the tokens are. If more security is needed, set this value upto 8!
};

}

#endif /* COAP_LAYER_H_ */
