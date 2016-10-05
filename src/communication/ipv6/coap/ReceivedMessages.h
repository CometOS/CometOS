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
 * @author: Martin Ringwelski
 */


#ifndef RECEIVEDMESSAGES_H_
#define RECEIVEDMESSAGES_H_

#include "CoAPMessage.h"
#include "coapconfig.h"

namespace cometos_v6 {

struct ReceivedMessage_t {
    IPv6Address ip;
    Token       token;
    uint16_t    msgId;
    CoapType_t  type;
    uint16_t    srcPort;
    uint16_t    dstPort;
    uint8_t     lifeTime;
    ReceivedMessage_t(const CoAPMessage* msg) :
        ip(msg->getAddr()),
        token(msg->getMessageToken()),
        msgId(msg->getMessageID()),
        type(msg->getMessageType()),
        srcPort(msg->getMessageSrcPort()),
        dstPort(msg->getMessageDstPort()),
        lifeTime(COAP_MESSAGE_LIFETIME)
    {
    }

    bool operator==(const CoAPMessage* msg) {
        return (msg->getAddr() == ip &&
                msg->getMessageSrcPort() == srcPort &&
                msg->getMessageDstPort() == dstPort &&
                msg->getMessageToken() == token &&
                msg->getMessageID() == msgId);
    }

    bool isAnswerTo(const CoAPMessage* msg) {
        return (msg->getAddr() == ip &&
                msg->getMessageSrcPort() == dstPort &&
                msg->getMessageDstPort() == srcPort &&
                msg->getMessageToken() == token);
    }

    bool isAckTo(const CoAPMessage* msg) {
        return (msg->getAddr() == ip &&
                msg->getMessageSrcPort() == dstPort &&
                msg->getMessageDstPort() == srcPort &&
                msg->getMessageID() == msgId);
    }
};

template<uint8_t N>
class ReceivedMessages {
public:
    ReceivedMessages() {
        for (uint8_t i = 0; i < N; i++) {
            rcvdMessages[i] = NULL;
        }
    }
    bool didReceiveMessage(const CoAPMessage* Message) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL && *(rcvdMessages[i]) == Message) {
                return true;
            }
        }
        return false;
    }
    bool didReceiveResponse(const CoAPMessage* Query) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL && rcvdMessages[i]->isAnswerTo(Query)) {
                return true;
            }
        }
        return false;
    }
    ReceivedMessage_t* getReceivedACK(const CoAPMessage* Msg) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL && rcvdMessages[i]->isAckTo(Msg)) {
                return rcvdMessages[i];
            }
        }
        return NULL;
    }
    void insertReceivedMessage(CoAPMessage* received){
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] == NULL) {
                rcvdMessages[i] = new ReceivedMessage_t(received);
                return;
            }
        }
    }
    void deleteReceivedResponse(const CoAPMessage* Query) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL && rcvdMessages[i]->isAnswerTo(Query)) {
                delete rcvdMessages[i];
                rcvdMessages[i] = NULL;
            }
        }
    }
    void deleteReceivedACK(const CoAPMessage* Msg) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL && rcvdMessages[i]->isAckTo(Msg)) {
                delete rcvdMessages[i];
                rcvdMessages[i] = NULL;
            }
        }
    }
    bool MsgIdReceived(const IPv6Address& addr, uint16_t srcPort, uint16_t dstPort, uint16_t MessageID) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL &&
                    rcvdMessages[i]->ip == addr &&
                    rcvdMessages[i]->srcPort == srcPort &&
                    rcvdMessages[i]->dstPort == dstPort &&
                    rcvdMessages[i]->msgId == MessageID) {
                return true;
            }
        }
        return false;
    }

    bool TokenReceived(const IPv6Address& addr, uint16_t srcPort, uint16_t dstPort, const Token& token) {
        for (uint8_t i = 0; i < N; i++) {
            if (rcvdMessages[i] != NULL &&
                    rcvdMessages[i]->ip == addr &&
                    rcvdMessages[i]->srcPort == srcPort &&
                    rcvdMessages[i]->dstPort == dstPort &&
                    rcvdMessages[i]->token == token) {
                return true;
            }
        }
        return false;
    }

    void decreaseTimeAndDelete() {
        for(uint8_t i = 0; i < N; i++)
        {
            if (rcvdMessages[i] != NULL){
                if (rcvdMessages[i]->lifeTime > 0) {
                    rcvdMessages[i]->lifeTime--;
                } else {
                    delete rcvdMessages[i];
                    rcvdMessages[i] = NULL;
                }
            }
        }
    }

    void deleteAll() {
        for(uint8_t i = 0; i < N; i++)
        {
            if(rcvdMessages[i] != NULL){
                delete rcvdMessages[i];
                rcvdMessages[i] = NULL;
            }
        }
    }

private:
    ReceivedMessage_t* rcvdMessages[N];
};


}

#endif /* RECEIVEDMESSAGES_H_ */
