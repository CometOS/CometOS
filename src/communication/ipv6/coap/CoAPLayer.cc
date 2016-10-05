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

#include "CoAPLayer.h"
#include "palId.h"
#include "CoAPResource.h"


namespace cometos_v6 {

const char * const CoAPLayer::MODULE_NAME = "CoAP";
const char* CoAPLayer::RESOURCE_DISCOVERY_PATH = "/.well-known/core";
const uint16_t CoAPLayer::RETRANSMIT_INTERVAL = 1000;

Define_Module(CoAPLayer);

CoAPLayer::CoAPLayer(const char * service_name) :
            cometos::Module(service_name),
            udp(NULL),
            rcvdMessages(),
            sentMessages(),
            currMessageID(intrand(0xFFFF) % 0xFFFF),
            counter(0),
            maxTokenLen(2) {
}

void CoAPLayer::initialize() {
	if (udp == NULL) {
		udp = (cometos_v6::UDPLayer*) getModule(UDP_MODULE_NAME);
		LOG_DEBUG("CoAPLayer Init");
	}
}

void CoAPLayer::finish(){
    deleteAllMessages();
    cancel(&timer);
}

bool CoAPLayer::unbindPort(uint16_t port)
{
    if (port != 0) {

        CoAPCallback_t* listener = getCorrespondingListener(responseListeners,
                                                            port);

        if (listener != NULL) {
            LOG_DEBUG("Remove Listener of Port " << port);
            listener->port = 0;
            listener->handler = NULL;
            return true;
        }
    }
    return false;
}

CoAPCallback_t* CoAPLayer::getCorrespondingListener(
        CoAPCallback_t listeners[COAP_MAX_LISTENERS], uint16_t port)
{
    for(uint8_t i = 0; i < COAP_MAX_LISTENERS; i++) {
        if(listeners[i].port == port) {
            return &(listeners[i]);
        }
    }

    return NULL;
}


/*
 *      Client has to call this function.
 */
uint8_t CoAPLayer::sendMessage(
        const URL& url,
        uint16_t srcPort,
        uint8_t messageType,
        uint8_t messageCode,
        CoAPMessageOption** options, uint8_t optionsLen,
        const char* payload, uint16_t payloadLen,
        bool handleOptions, bool handlePayload,
        uint8_t tag)
{
    uint8_t errcode = 0;
    if (messageType == COAP_ACK ||
            messageType == COAP_RST ||
            messageCode > 0x1F)
    {
        return COAP_MESSAGE_ILLEGAL_TYPE_OR_CODE;
    }

    CoAPMessage* message = prepareMessage();
    if (message == NULL) {
        return COAP_MESSAGE_SILENTIGNORE;
    }

    LOG_DEBUG("url: " << url.toString());

    if (url.ip.isUnspecified()) {
    	delete message;
        return COAP_MESSAGE_URL_UNREC_ADDRESS;
    }

    CoAPMessageOption* optionList[COAP_MAX_MESSAGE_OPTIONS];
    memset(optionList, 0, sizeof(CoAPMessageOption*)*COAP_MAX_MESSAGE_OPTIONS);

    for(uint8_t i = 0; i < optionsLen; i++) {
        optionList[i] = options[i];
    }

    Token token;
    if ((messageType == COAP_CON || messageType == COAP_NON) &&
            messageCode < 0x20)
    {  //If CON or NON and a request
        token = getNextToken(
                message->getAddr(),
                message->getMessageSrcPort(),
                message->getMessageDstPort(),
                maxTokenLen);
    }

    uint16_t messageID = getNextMessageID(
            message->getAddr(),
            message->getMessageSrcPort(),
            message->getMessageDstPort());

    if (message->init(url,
            srcPort,
            messageType,
            messageCode,
            messageID,
            token,
            optionList, COAP_MAX_MESSAGE_OPTIONS,
            payload, payloadLen,
            handleOptions, handlePayload) != 0)
    {
        errcode = message->getMessageError();
        delete message;
        return errcode;
    }

    message->setMessageTag(tag);

    //and try to send it without handling the data;
    errcode = sendMessage(message, false);
    if (errcode) {
    	delete message;
    } else {
        LOG_INFO("Send to UDP Layer");
    }

    return errcode;
}

void CoAPLayer::initMessage(
        CoAPMessage* msg,
        const URL& url,
        uint16_t srcPort,
        uint8_t messageType,
        uint8_t messageCode,
        uint16_t messageID,
        Token token,
        CoAPMessageOption** options, uint8_t optionsLen,
        const char* payload, uint16_t payloadLen,
        bool handleOptions, bool handlePayload)
{
    if(msg == NULL) {
        return;
    }

    //int error =
    msg->init(
            url,
            srcPort,
            messageType,
            messageCode,
            messageID,
            token,
            options, optionsLen,
            payload, payloadLen,
            handleOptions, handlePayload);

    if(msg->getMessagePacket() == NULL){
        delete msg;
        deleteAllMessages();
        msg = prepareMessage();
        msg->init(url,
                srcPort,
                messageType,
                messageCode,
                messageID,
                token,
                options, optionsLen,
                payload, payloadLen,
                handleOptions, handlePayload);
    }
}

bool CoAPLayer::isResourceDiscovery(CoAPMessage* request){

    char* path = NULL;
    uint16_t pathLen = request->getPathString(path);

    if (pathLen >= 17) {

        if(strncmp(path, RESOURCE_DISCOVERY_PATH, 17) == 0) {
            delete[] path;
            return true;
        }
    }

    delete[] path;
    return false;
}

void CoAPLayer::udpPacketReceived(
        const IPv6Address& src, uint16_t srcPort, uint16_t dstPort,
        const uint8_t* data, uint16_t length)
{
    ENTER_METHOD_SILENT();

    CoAPMessage* message = new CoAPMessage(
            &buffer, src, srcPort, dstPort, data, length);
    int8_t err = message->getMessageError();
    if (err) {
        LOG_ERROR("Error: " << (int)err);
        deleteAllMessages();
        delete message;
        return;
    }
    int messageErrCode = message->decodeCoAPMessage(NULL, 0);

    LOG_DEBUG(" <-- " << message->getMessageString());
//    message->printMessage(palId_id(), true);

    //sets the meaning of the message code
    CoAPMessage::codeMeaning_t meaning;
    if(message->getMessageCodeMeaning(meaning) != 0) {
        messageErrCode |= COAP_MESSAGE_FORMATERROR;
    }

    if ((messageErrCode == COAP_MESSAGE_SILENTIGNORE) ||
            (messageErrCode & COAP_MESSAGE_UNREC_OPTION)){
        LOG_WARN("Error: " << (int)messageErrCode);
        delete message;
        return;
    }
    else if((messageErrCode & COAP_MESSAGE_FORMATERROR) &&
            (message->getMessageType() == COAP_CON)
            /*|| mesage.getMessageType() == COAP_NON && COAP_SEND_RST_ON_NON*/)
    {
        LOG_WARN("Error: " << (int)messageErrCode);
        sendResetMessage(message, dstPort);
        delete message;
        return;
    }

    IPv6Address addr = message->getAddr();
    uint16_t mID = message->getMessageID();
    Token token = message->getMessageToken();

    switch(message->getMessageType()){
        case COAP_ACK:
            LOG_INFO("ACK from [" << src.str().c_str() << "]:" << srcPort);
            if (meaning == CoAPMessage::EMPTY ||
                    meaning == CoAPMessage::RESPONSE)
            {
                if (sentMessages.didSendRequest(addr,
                        dstPort, srcPort, true, true, mID, token))
                {   //If we sent a matching request
                    //insertReceivedMessage(message);
                    handleResponse(message, addr, srcPort, dstPort, true,
                            false, true, 0, token);
                } else {
                    LOG_INFO("Response to unknown request");
                }
            }
            delete message;
            break;
        case COAP_CON:
            if(meaning == CoAPMessage::EMPTY){
                LOG_INFO("CON [" << src.str().c_str() << "]:" << srcPort);
                //Empty CON Message are prohibited and need a RST Message as a response
                sendResetMessage(message, dstPort);
                delete message;
            }
            else if(meaning == CoAPMessage::RESPONSE){
                LOG_INFO("CON Resp from [" << src.str().c_str() << "]:" << srcPort);
                if(sentMessages.didSendRequest(
                        addr, dstPort, srcPort, false, true, 0, token))
                {   //If we sent a matching request
                    //Acknowledge the receipt, delete the request and call the handler
                    sendAckMessage(message, dstPort);
                    //insertReceivedMessage(message);
                    LOG_DEBUG("Call Handler");
                    handleResponse(message, addr, srcPort, dstPort, true,
                            false, true, 0, token);
                }
                else{
                    //If we lack the context for this response send a RST message
                	LOG_INFO("Lack of context");
                    sendResetMessage(message, dstPort);
                }
                delete message;
            }
            else if(meaning == CoAPMessage::REQUEST){
                LOG_INFO("CON Req from [" << src.str().c_str() << "]:" << srcPort);
                LOG_DEBUG("Search Request Handler");
                //Try to find the matching resource
                CoAPResource* resource =
                        resources.findMatchingResource(message);
                //but if there is none
                if(resource == NULL){
                    //and the URI path is /well-known/core lookup all visible resources
                    if(isResourceDiscovery(message))
                    {
                        char* payload = NULL;
                        //and send their paths as a response
                        uint16_t pathLength =
                                resources.getResourceDiscoveryList(payload);

                        //if there are no resources visible for resource discovery
                        if(pathLength == 0) {
                            delete message;
                            //then stop this.
                            return;
                        }

                        sendPiggybackMessage(message,
                                dstPort,
                                payload, strlen(payload));

                        delete[] payload;

                    } else {
                        LOG_INFO("No Res Found");
                    }
                    delete message;
                }
                else
                {
                    if(rcvdMessages.didReceiveMessage(message)) {
                        LOG_INFO("CON dup from [" << src.str().c_str() << "]:" << srcPort);
                        //If we already received this exact message
                        if(sentMessages.didSendResponse(
                                addr, srcPort, dstPort, true, true, mID, token))
                        {
                            //and we already sent a piggy back response
                            resource->handleDuplicateRequest(
                                    message, COAP_DUPLICATE_REQ_PIGGYBACK_SENT);
                        } else if(sentMessages.didSendResponse(
                                addr, srcPort, dstPort, false, true, 0, token))
                        {
                            //or an ACK and the matching seperate respose
                            resource->handleDuplicateRequest(
                                    message, COAP_DUPLICATE_REQ_SEPERATE_SENT);
                        } else if(sentMessages.didSendMessage(
                                addr, srcPort, dstPort, true, false, mID))
                        {
                            //or an ACK but not a matching seperate response
                            sendAckMessage(message, dstPort);
                        } else {
                            //or if we didn't send anything yet
                            resource->handleDuplicateRequest(
                                    message, COAP_DUPLICATE_REQ_NOTHING_SENT);
                        }
                        delete message;
//                        message->deleteMessageBody();
                    }
                    else {
                        rcvdMessages.insertReceivedMessage(message);
                        LOG_DEBUG("Call Resource Handler");
                        resource->handleRequest(message);
                        delete message;
//                        message->deleteMessageBody();
                        if (!isScheduled(&timer)) {
                            schedule(&timer,
                                    &CoAPLayer::retransmitTimerFired,
                                    RETRANSMIT_INTERVAL);
                        }
                    }
                }
            }
            break;
        case COAP_NON:
            if(meaning == CoAPMessage::RESPONSE){
                LOG_INFO("NON Resp from [" << src.str().c_str() << "]:" << srcPort);
                //if we have sent the matching request
                if (sentMessages.didSendRequest(
                        addr, dstPort, srcPort, false, true, 0, token))
                {
                    //insertReceivedMessage(message);

                    handleResponse(message, addr, srcPort, dstPort, true,
                            false, true, 0, token);

                } else {
                    LOG_INFO("unrec resp");
                }
                delete message;
            }
            else if(meaning == CoAPMessage::REQUEST){
                LOG_INFO("NON Req from [" << src.str().c_str() << "]:" << srcPort);
                //Try to find the matching resource
                CoAPResource* resource =
                        resources.findMatchingResource(message);
                if(resource == NULL) {
                    if(isResourceDiscovery(message))
                    {
                        LOG_INFO("RD");

                        char* payload = NULL;
                        //and send their paths as a response
                        uint16_t pathLength =
                                resources.getResourceDiscoveryList(payload);

                        //if there are no resources visible for resource discovery
                        if(pathLength == 0) {
                            LOG_INFO("No Resources");
                            delete message;
                            //then stop this.
                            return;
                        }

                        sendResponse(message, dstPort,
                                COAP_NON,
                                COAP_CONTENT,
                                getNextMessageID(addr, srcPort, dstPort),
                                message->getMessageToken(),
                                NULL, 0,
                                payload, strlen(payload),
                                false, false);

                        delete[] payload;

                    } else {
                        LOG_INFO("No Res Found");
                    }
                    delete message;
                    return;
                }
                //If we already received this exact message
                if(rcvdMessages.didReceiveMessage(message)) {
                    //or an ACK and the matching seperate respose
                    if(sentMessages.didSendResponse(
                            addr, srcPort, dstPort, false, true, 0, token))
                    {
                        resource->handleDuplicateRequest(
                                message, COAP_DUPLICATE_REQ_SEPERATE_SENT);
                    } else {
                        //or if we didn't send anything yet
                        resource->handleDuplicateRequest(
                                message, COAP_DUPLICATE_REQ_NOTHING_SENT);
                    }
                    delete message;
//                    message->deleteMessageBody();
                } else {
                    rcvdMessages.insertReceivedMessage(message);
                    resource->handleRequest(message);
                    delete message;
                    if (!isScheduled(&timer)) {
                        schedule(&timer,
                                &CoAPLayer::retransmitTimerFired,
                                RETRANSMIT_INTERVAL);
                    }
//                    message->deleteMessageBody();
                }
            }
            break;
        case COAP_RST:
            LOG_INFO("RST from [" << src.str().c_str() << "]:" << srcPort);
            if(meaning == CoAPMessage::EMPTY){
                if (sentMessages.didSendMessage(
                        addr, dstPort, srcPort, true, false, mID))
                {   //if we indeed sent the message (request or response or empty)
                    //insertReceivedMessage(message);
                    handleResponse(message, addr, srcPort, dstPort, false,
                            true, false, mID);

                }
                //if we didn't sent the message
            }
            //Ignore all other RST Messages
            delete message;
            break;
    }

}

/*
 * Runs through all sent messages and looks for CONs that have to be resent or can be deleted.
 * Sets the unresponded request inactive after COAP_MAX_RETRANSMIT and deletes them
 * after COAP_EXCHANGE_LIFETIME - alreadyWaitedTime, so that they were alive for at most COAP_EXCHANGE_TIME.
 */
void CoAPLayer::retransmitTimerFired(cometos::Message* msg){

    for(uint8_t i = 0; i < COAP_MAX_MESSAGE_BUFFER; i++)
    {
        CoAPMessage* msg = sentMessages.getEntry(i);
        if(msg != NULL){

            if(msg->getMessageType() == COAP_CON)
            {
                //If this message can be retransmitted and its timeout triggered
                if(msg->getMessageTimer() <= COAP_RETRANSMIT_INTVAL &&
                        msg->getMessageRetries() < COAP_MAX_RETRANSMIT)
                {
                    ReceivedMessage_t* receivedRstAck =
                            rcvdMessages.getReceivedACK(msg);
                    //If we already have the response or have gotten an RST
                    if(rcvdMessages.didReceiveResponse(msg))
                    {
                        sentMessages.recalculateMessageTimer(msg);
                    }
                    else if(receivedRstAck != NULL &&
                            receivedRstAck->token.isZero() &&
                            (receivedRstAck->type == COAP_RST ||
                                    receivedRstAck->type == COAP_ACK))
                    {
                        rcvdMessages.deleteReceivedACK(msg);

                        sentMessages.deleteMessage(
                                msg->getAddr(),
                                msg->getMessageSrcPort(),
                                msg->getMessageDstPort(),
                                false, true,
                                0,
                                msg->getMessageToken());
                    }
                    //if we didnt receive an response yet, resend it
                    else if(msg->getMessageRetries() < COAP_MAX_RETRANSMIT - 1)
                    {
                        LOG_INFO("Resending "
                                << (msg->getMessageRetries() + 1)
                                << " th times");
                        //send the message
                        sendMessage(msg, false);

                        //increment the retries
                        msg->incrementMessageRetries(1);
                        //set the new timer value
                        msg->setMessageTimer();
                    }
                    else
                    {
                        //send it a last time and make sure the counter and timer are ok!
                        LOG_INFO("Resending "
                                << (msg->getMessageRetries() + 1)
                                << " th times");
                        //send the message
                        sendMessage(msg, false);
                        sentMessages.recalculateMessageTimer(msg);
                    }
                }
                //if this message doesnt need to be resend just now, decrement its timer value
                else if(msg->getMessageRetries() < COAP_MAX_RETRANSMIT)
                {
                    msg->incrementMessageTimer(- COAP_RETRANSMIT_INTVAL);
                }
                //the message awaits deletion after we can safely assume we won't need it anymore
                else if(msg->getMessageRetries() == COAP_MAX_RETRANSMIT)
                {
                    sentMessages.removeSentOrIncreaseTimer(i);
                }
            }
            else if(msg->getMessageType() == COAP_NON)
            {
                sentMessages.removeSentOrIncreaseTimer(i);
            }
        }

    }
    /*
     * Runs through the list of received messages and deletes them as soon as
     * they lived for their lifetime.
     */
    rcvdMessages.decreaseTimeAndDelete();

#ifdef LOWPAN_ENABLE_BUFFERSTATS
    LOG_DEBUG("Buffer used: " << buffer.getUsedBufferSize()
            << " Entries: " << (int)buffer.getNumBuffers());
#endif

    if (!isScheduled(&timer)) {
        schedule(&timer, &CoAPLayer::retransmitTimerFired, RETRANSMIT_INTERVAL);
    }

}
/*
 * Sends the CoAP message via UDP to the endpoint specified inside the message.
 * This function copies the message and optionally deletes the callers copy.
 * The internal copy will be deleted as soon as the message isn't used anymore.
 *
 * The CoAPLayer needs to preserve each message for about 250 second for CONs
 * and 150s for NONs as to be able to resend it and detect duplicates.
 *
 * Attention: If handleData is set to true, this function will delete the callers copy even if
 * sending failed!
 */
uint8_t CoAPLayer::sendMessage(CoAPMessage* message, bool handleData) {
    ENTER_METHOD_SILENT();

    if ((message->getMessageType() != COAP_ACK &&
            sentMessages.isFull() &&
            !sentMessages.didSendMessage(
                    message->getAddr(),
                    message->getMessageSrcPort(),
                    message->getMessageDstPort(),
                    true, false,
                    message->getMessageID()))
            || message->getMessagePacket() == NULL)
    {
        LOG_WARN("Queue Full");
        if (handleData) {
            delete message;
        }
        return COAP_MESSAGE_QUEUE_FULL;
    }

    uint16_t srcPort = message->getMessageSrcPort();
    uint16_t dstPort = message->getMessageDstPort();
    const uint8_t* data = message->getMessagePacket();
    uint16_t dataLen = message->getMessagePacketLen();

    ASSERT(udp != NULL);
    LOG_DEBUG(" --> " << message->getMessageString());
//    message->printMessage(palId_id(), false);

    if (message->getMessageType() != COAP_CON) {
        message->onlyKeepMetaData();
    }

    if (udp->sendMessage(message->getAddr(),
            srcPort, dstPort, data, dataLen, NULL))
    {
        LOG_INFO("CoAP->UDP succ");

        if(message->getMessageType() != COAP_RST && message->getMessageType() != COAP_ACK){

            //We don't need to save RSTs and ACKs, as their MIDs dont belong to us.
            if(!sentMessages.didSendMessage(
                    message->getAddr(),
                    message->getMessageSrcPort(),
                    message->getMessageDstPort(),
                    true, true,
                    message->getMessageID(),
                    message->getMessageToken()))
            {
                if (sentMessages.add(message)) {
                    LOG_INFO("Added Msg to SendBuffer");
                } else {
                    LOG_WARN("Could not Add Msg to SendBuffer");
                }
            }

            if (!isScheduled(&timer)) {
                schedule(&timer, &CoAPLayer::retransmitTimerFired, RETRANSMIT_INTERVAL);
            }
        } else {
            delete message;
            message = NULL;
        }
    } else {
        LOG_WARN("CoAP->UDP failed");
        if(handleData && sentMessages.didSendMessage(
                message->getAddr(),
                message->getMessageSrcPort(),
                message->getMessageDstPort(),
                true, true,
                message->getMessageID(),
                message->getMessageToken()))
        {
            delete message;
            message = NULL;
        }
    }
    if(handleData && message != NULL){
        if(!sentMessages.didSendMessage(
                message->getAddr(),
                message->getMessageSrcPort(),
                message->getMessageDstPort(),
                true, true,
                message->getMessageID(),
                message->getMessageToken()))
        {
            delete message;
        } else {
            LOG_INFO("Delete Msg");
        	sentMessages.remove(message);
        }
    }

    return 0;
}

uint16_t CoAPLayer::getNextMessageID(
        const IPv6Address& addr, uint16_t srcPort, uint16_t dstPort)
{

    do  //get new message ids until there is an unused one found!
    {
        currMessageID++;
    } while(rcvdMessages.MsgIdReceived(addr, dstPort, srcPort, currMessageID) ||
            sentMessages.didSendMessage(addr,
                    srcPort, dstPort, true, false, currMessageID));

    return currMessageID;
}

Token CoAPLayer::getNextToken(
        const IPv6Address& addr,
        uint16_t srcPort,
        uint16_t dstPort,
        uint8_t length)
{
    Token token;

    if(length > Token::MAX_LENGTH || length == 0)
        return token;

    do  //get new tokens until there is an unused one found!
    {
        token.generate(length);

    } while(rcvdMessages.TokenReceived(addr, dstPort, srcPort, token) ||
            sentMessages.didSendMessage(addr,
                    srcPort, dstPort, false, true, 0, token));

    return token;
}

void CoAPLayer::handleResponse(CoAPMessage* message, const IPv6Address& addr,
            uint16_t srcPort, uint16_t dstPort, bool state,
            bool byMessageID, bool byToken,
            uint16_t messageID, Token token) {
        CoAPMessage* sMsg = sentMessages.getSendMessage(
                addr,
                dstPort,
                srcPort,
                byMessageID, byToken,
                messageID, token);
        message->setMessageTag(sMsg->getMessageTag());
        CoAPCallback_t* listener = getCorrespondingListener(responseListeners, dstPort);
        if (listener != NULL) {
            LOG_DEBUG("Call Response Handler");
            listener->handler->incommingCoAPHandler(message, state);
        } else {
            LOG_WARN("No Response Handler Found");
        }

        sentMessages.deleteMessage(addr, dstPort, srcPort, byMessageID, byToken, messageID, token);

//        message->deleteMessageBody();
    }

//-------------------- List managing functions ---------------------------------

void CoAPLayer::deleteAllMessages(){
    LOG_INFO("Deleting all Msgs");
    rcvdMessages.deleteAll();
    sentMessages.deleteAll();
//    buffer.clearAll();
}



}
