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

#ifndef SENTMESSAGES_H_
#define SENTMESSAGES_H_

#include "CoAPMessage.h"
#include "coapconfig.h"

namespace cometos_v6 {

template<uint8_t N>
class SentMessages {
public:
    SentMessages(): sentMessages{NULL} {
//        for (uint8_t i = 0; i < N; i++) {
//            sentMessages[i] = NULL;
//        }
    }

    void removeSentOrIncreaseTimer(uint8_t i) {
        //message can safely be removed
        if(sentMessages[i]->getMessageTimer() <= COAP_RETRANSMIT_INTVAL)
        {
            deleteMessage(
                    sentMessages[i]->getAddr(),
                    sentMessages[i]->getMessageSrcPort(),
                    sentMessages[i]->getMessageDstPort(),
                    false, true,
                    0,
                    sentMessages[i]->getMessageToken(), false);

            remove(sentMessages[i]);
        }
        //still have to wait
        else
        {
            sentMessages[i]->incrementMessageTimer( -1 );
        }
    }

    bool didSendResponse(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token()){
        CoAPMessage* message = getSendMessage(
                addr, srcPort, dstPort, byMessageID, byToken, messageID, token);
        if(message != NULL)
            return message->isResponse();
        return false;
    }
    bool didSendRequest(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token()){
        CoAPMessage* message = getSendMessage(
                addr, srcPort, dstPort, byMessageID, byToken, messageID, token);
        if(message != NULL) {
            return message->isRequest();
        }
        return false;
    }
    bool didSendMessage(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token()){
        CoAPMessage* message = getSendMessage(
                addr, srcPort, dstPort, byMessageID, byToken, messageID, token);
        if(message != NULL)
            return true;
        return false;
    }

    CoAPMessage* getSendMessage(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token()){
        return find(
                addr, srcPort, dstPort, messageID, token, byMessageID, byToken);
    }

    bool add(CoAPMessage* add) {
        if(find(add->getAddr(),
                add->getMessageSrcPort(),
                add->getMessageDstPort(),
                add->getMessageID(),
                add->getMessageToken(),
                true, true) != NULL)
        {
            return true;
        }

        for(uint8_t i = 0; i < N; i++){
            if(sentMessages[i] == NULL){
                if (add->getMessageType() == COAP_NON) {
                    add->setMessageTimer(COAP_NON_LIFETIME);
                }
                sentMessages[i] = add;
                return true;
            }
        }
        return false;
    }

    bool remove(CoAPMessage* msg) {

        for(uint8_t i = 0; i < N; i++){
            if(sentMessages[i] == msg){
                sentMessages[i] = NULL;
                delete msg;
                return true;
            }
        }
        return false;
    }

    bool isFull() {

        for(uint8_t i = 0; i < N; i++) {
            if(sentMessages[i] == NULL) {
                return false;
            }
        }
        return true;
    }

    bool deleteMessage(const IPv6Address& addr, uint16_t srcPort,
            uint16_t dstPort, bool byMessageID, bool byToken,
            uint16_t messageID = 0, Token token = Token(), bool noMulticast = true)
    {
        CoAPMessage* found = find(
                addr, srcPort, dstPort, messageID, token, byMessageID, byToken);
        if(found == NULL || (noMulticast && found->getAddr().isMulticast())) {
            return false;
        }
        remove(found);
        return true;
    }

    CoAPMessage* find(
            const IPv6Address& addr,
            uint16_t srcPort,
            uint16_t dstPort,
            uint16_t messageID,
            Token token,
            bool tryMID,
            bool tryToken) {

        for (uint8_t i = 0; i < N; i++) {
            if(sentMessages[i] != NULL) {
                if((sentMessages[i]->getAddr() == addr || sentMessages[i]->getAddr().isMulticast()) &&
                        sentMessages[i]->getMessageDstPort() == dstPort &&
                        sentMessages[i]->getMessageSrcPort() == srcPort)
                {
                    if (((sentMessages[i]->getMessageID() == messageID) || !tryMID) &&
                       ((sentMessages[i]->getMessageToken() == token) || !tryToken))
                    {
                        return sentMessages[i];
                    }
                }
            }
        }

        return NULL;
    }

    void deleteAll() {
        for(uint8_t i = 0; i < N; i++) {
            if(sentMessages[i] != NULL) {
                delete sentMessages[i];
                sentMessages[i] = NULL;
            }
        }
    }

    void recalculateMessageTimer(CoAPMessage* msg) {
        //set the retries too high to let it being ignored by this loop and the
        uint8_t waitedTime = 0;
        for(uint8_t j = 0; j < msg->getMessageRetries() + 1; j++) {
            waitedTime += msg->calculateTimer(j);
        }

        msg->setMessageRetries(COAP_MAX_RETRANSMIT);
        //reset timer and increment to automatically handle negative timer value, as setMessageTimer only accepts uint8_t
        msg->setMessageTimer(0);
        msg->incrementMessageTimer(COAP_EXCHANGE_LIFETIME - waitedTime);
    }

    CoAPMessage* getEntry(uint8_t i) {
        if (i < N) return sentMessages[i];
        return NULL;
    }
protected:
    CoAPMessage* sentMessages[N];
};

}

#endif /* SENTMESSAGES_H_ */
