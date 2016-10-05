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


#include "ICMPv6.h"
#include "ICMPv6Message.h"
#include "LowpanAdaptionLayer.h"

namespace cometos_v6 {

Define_Module(ICMPv6);

ICMPv6::ICMPv6(const char * service_name) :
        cometos::Module(service_name),
        fromIP(this, &ICMPv6::handleIPRequest, "fromIP"),
        toIP(this, "toIP")
{
    for (uint8_t i = 0; i < ICMP_MAX_CONTENTREQUESTS; i++) {
        contentRequests[i].contentRequest.content =
                (FollowingHeader*)(new ICMPv6Message());
        contentRequests[i].contentRequest.setResponseDelegate(
                createCallback(&ICMPv6::handleIPResponse));
    }
}

void ICMPv6::initialize() {
    schedule(&timeOutSchedule,
            &ICMPv6::handleTimeout,
            ICMP_CHECK_TIMEOUT);
}

void ICMPv6::handleTimeout(cometos::Message *msg) {
    ENTER_METHOD_SILENT();
    for (uint8_t i = 0; i < ICMP_ECHO_MAX_HANDLERS; i++) {
        if (echoHandlers[i].handler != NULL) {
            if (echoHandlers[i].t == 1) {
                echoHandlers[i].handler->callHandler(echoHandlers[i].seqNum, false);
                delete echoHandlers[i].handler;
                echoHandlers[i].handler = NULL;
                echoHandlers[i].id = 0;
                echoHandlers[i].seqNum = 0;
            } else {
                echoHandlers[i].t++;
            }
        }
    }
    schedule(msg,
            &ICMPv6::handleTimeout,
            ICMP_CHECK_TIMEOUT);
}

void ICMPv6::handleIPRequest(ContentRequest *cRequest) {
    ENTER_METHOD_SILENT();
    FollowingHeader *fh = cRequest->content;
    bool resp = false;
    if (fh->getHeaderType() == ICMPv6Message::HeaderNumber) {
        ICMPv6Message* icmp = static_cast<ICMPv6Message*>(fh);
        if (icmp->checkValid(cRequest->src, cRequest->dst)) {
            LOG_DEBUG("Rcvd ICMP Msg Type " << (uint16_t)icmp->getType());
            for (uint8_t idx = listeners.begin(); idx != listeners.end(); idx = listeners.next(idx)) {
                LOG_DEBUG("Call ICMP Listener");
                icmpListenerHandler_t handler = listeners.get(idx);
                if (handler.type == -1 || handler.type == icmp->getType()){
                    handler.listener->icmpMessageReceived(cRequest->src,
                                                    cRequest->dst,
                                                    icmp->getType(),
                                                    icmp->getCode(),
                                                    icmp->getAdditionalFields(),
                                                    icmp->getData(),
                                                    icmp->getUpperLayerPayloadLength());
                    resp = true;
                }
            }
//            for (uint8_t i = 0; i < ICMP_MAX_LISTENERS; i++) {
//                if (listeners[i].handler != NULL &&
//                        (listeners[i].type == -1 ||
//                         ((uint8_t)listeners[i].type) == icmp->getType())) {
//                    LOG_DEBUG("Call ICMP Listener");
//                    listeners[i].handler->callHandler(
//                            cRequest->src,
//                            cRequest->dst,
//                            icmp->getType(),
//                            icmp->getCode(),
//                            icmp->getAdditionalFields(),
//                            icmp->getData(),
//                            icmp->getDataLength());
//                }
//            }

            switch (icmp->getType()) {
            case ICMP_TYPE_ECHO_REQUEST: {
                LOG_DEBUG("Rcvd ECHOREQ from " << cRequest->src.getAddressPart(7));
                if (!cRequest->src.isMulticast()) {
                    IPv6Address tmp = cRequest->dst;
                    cRequest->dst = cRequest->src;
                    if (tmp.isMulticast()) {
                        cRequest->src.set(0,0,0,0,0,0,0,0);
                    } else {
                        cRequest->src = tmp;
                    }

                    icmp->setType(ICMP_TYPE_ECHO_REPLY);
                    LOG_DEBUG("Send ECHOREP to " << cRequest->dst.getAddressPart(7));
                    toIP.send(cRequest);
                    resp = true;
                }
                break;
            }
            case ICMP_TYPE_ECHO_REPLY: {
                LOG_DEBUG("Rcvd ECHOREP!");
                uint8_t i;
                uint16_t id = icmp->getAdditionalFields()[0];
                uint16_t seqNum = icmp->getAdditionalFields()[1];
                for (i = 0;
                        i < ICMP_ECHO_MAX_HANDLERS &&
                        (echoHandlers[i].id != id ||
                        echoHandlers[i].seqNum != seqNum ||
                        echoHandlers[i].handler == NULL);
                     i++);
                if (i < ICMP_ECHO_MAX_HANDLERS) {
                    LOG_DEBUG("Call ECHO Hndlr of Pos " << (uint16_t)i);
                    echoHandlers[i].handler->callHandler(seqNum, true);
                    LOG_DEBUG("Remove ECHO Hndlr");
                    delete echoHandlers[i].handler;
                    echoHandlers[i].handler = NULL;
                    echoHandlers[i].id = 0;
                    echoHandlers[i].seqNum = 0;
                }
                resp = true;
                break;
            }
            default: {
                break;
            }
            }
        } else {
            LOG_WARN("Invalid Pckt");
        }
    } else {
        LOG_WARN("Not an ICMP Pckt");
    }
    cRequest->response(new ContentResponse(cRequest, resp));
}

void ICMPv6::handleIPResponse(ContentResponse *cResponse) {
    ENTER_METHOD_SILENT();
    if (cResponse->success) {
        LOG_DEBUG("ICMP Pckt send succ");
    } else {
        LOG_DEBUG("ICMP Pckt not send");
        ICMPv6Message* icmp = static_cast<ICMPv6Message*>(cResponse->refersTo->content);
        if (icmp->getType() == ICMP_TYPE_ECHO_REQUEST)
        {
            uint8_t i;
            uint16_t id = icmp->getAdditionalFields()[0];
            uint16_t seqNum = icmp->getAdditionalFields()[1];
            for (i = 0;
                    i < ICMP_ECHO_MAX_HANDLERS &&
                    (echoHandlers[i].id != id ||
                    echoHandlers[i].seqNum != seqNum);
                 i++);
            if (i < ICMP_ECHO_MAX_HANDLERS && echoHandlers[i].handler != NULL) {
                LOG_DEBUG("Call ECHO Hndlr");
                echoHandlers[i].handler->callHandler(seqNum, false);
                LOG_DEBUG("rm ECHO Hndlr");
                delete echoHandlers[i].handler;
                echoHandlers[i].handler = NULL;
                echoHandlers[i].id = 0;
                echoHandlers[i].seqNum = 0;
            }
        }
    }
    LOG_DEBUG("Free ContentRequest");
    freeContentRequest(cResponse->refersTo);
    delete cResponse;
}

//uint8_t ICMPv6::getNextFreeListener() {
//    int8_t i;
//    for (i = 0;
//            i < ICMP_MAX_LISTENERS && listeners[i].handler != NULL;
//         i++);
//    return i;
//}

uint8_t ICMPv6::getNextFreeEchoHandler() {
    uint8_t i;
    for (i = 0;
            i < ICMP_ECHO_MAX_HANDLERS && echoHandlers[i].handler != NULL;
            i++);
    return i;
}

//bool ICMPv6::deregisterListener(uint8_t number)
//{
//    if (listeners[number].handler != NULL) {
//        delete listeners[number].handler;
//        listeners[number].handler = NULL;
//        return true;
//    }
//    return false;
//}

bool ICMPv6::deregisterListener(ICMPv6Listener * listener, int16_t type) {
    icmpListenerHandler_t handler(listener, type);
    uint8_t res = listeners.find(handler);
    if (res != listeners.end()) {
        listeners.erase(res);
        return true;
    } else {
        return false;
    }
}

bool ICMPv6::registerListener(ICMPv6Listener * listener, int16_t type) {
    ENTER_METHOD_SILENT();
    icmpListenerHandler_t handler(listener, type);
    if (listeners.end() == listeners.find(handler)) {
        uint8_t res = listeners.push_front(handler);
        if (res != listeners.end()){
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool ICMPv6::sendErrorMessage(IPv6Datagram* datagram, uint8_t type, uint8_t code) {
    ENTER_METHOD_SILENT();
    if (!datagram->dst.isMulticast() && !datagram->src.isMulticast()) {
        if (datagram->getLastHeader()->getHeaderType() == ICMPv6Message::HeaderNumber) {
            uint8_t embtype = ((ICMPv6Message*)(datagram->getLastHeader()))->getType();
            switch (embtype) {
            case ICMP_TYPE_DESTINATION_UNREACHABLE:
            case ICMP_TYPE_PACKET_TOO_BIG:
            case ICMP_TYPE_TIME_EXCEEDED:
            case ICMP_TYPE_PARAMETER_PROBLEM:
                LOG_DEBUG("No ErrMsg");
                return true;
                break;
            default:
                ;;
            }
        }
        LowpanAdaptionLayer * lowpan = (cometos_v6::LowpanAdaptionLayer *) getModule(LOWPAN_MODULE_NAME);
        BufferInformation* bi = lowpan->getLowpanDataBuffer(datagram->getCompleteHeaderLength());
        if (bi == NULL) {
            LOG_ERROR("Buffer Full");
            return false;
        }
        uint16_t addData[2] = { 0, 0 };
        datagram->writeHeaderToBuffer(const_cast<uint8_t *>(bi->getContent()));
        return sendMessage(
                datagram->src,
                type,
                code,
                addData,
                bi);
    } else {
        LOG_DEBUG("No ErrMsg");
        return true;
    }
}

bool ICMPv6::sendMessage(const IPv6Address& dst,
        uint8_t type,
        uint8_t code,
        const uint16_t addData[2],
        const uint8_t* data,
        uint16_t length)
{
    ENTER_METHOD_SILENT();
    if (data != NULL && length > 0) {
        LowpanAdaptionLayer * lowpan = (cometos_v6::LowpanAdaptionLayer *) getModule(LOWPAN_MODULE_NAME);
        BufferInformation* bi = lowpan->getLowpanDataBuffer(length);
        if (bi == NULL) {
            LOG_ERROR("Buffer Full");
            return false;
        }
        bi->copyToBuffer(data, length);

        return sendMessage(dst, type, code, addData, bi);
    }
    return sendMessage(dst, type, code, addData, (BufferInformation*)NULL);
}

bool ICMPv6::sendMessage(const IPv6Address& dst,
        uint8_t type,
        uint8_t code,
        const uint16_t addData[2],
        BufferInformation* bi) {
    ENTER_METHOD_SILENT();
    ContentRequest* request = getContentRequest(bi);
    if (request == NULL) {
        LOG_ERROR("No free ContentReq");
        if (bi != NULL) bi->free();
        return false;
    }

    request->dst = dst;
    (static_cast<ICMPv6Message*>(request->content))->setType(type);
    (static_cast<ICMPv6Message*>(request->content))->setCode(code);
    (static_cast<ICMPv6Message*>(request->content))->setAdditionalFields(addData);
    if (bi != NULL) {
        (static_cast<ICMPv6Message*>(request->content))->setData(bi->getContent(), bi->getSize());
    } else {
        (static_cast<ICMPv6Message*>(request->content))->setData(NULL, 0);
    }

    LOG_DEBUG("Sending ICMP Pckt Type " << (uint16_t)type
            << ", Code " << (uint16_t)code
            << ", to " << dst.getAddressPart(7) << " cr:" << (uintptr_t)request);

    toIP.send(request);
    return true;

}

ContentRequest* ICMPv6::getContentRequest(BufferInformation* data) {
    ContentRequest* ret = NULL;
    uint8_t i;
    for (i = 0;
            i < ICMP_MAX_CONTENTREQUESTS && contentRequests[i].occupied;
            i++);
    if (i < ICMP_MAX_CONTENTREQUESTS) {
        LOG_DEBUG("ContentReq acquired");
        ret = &(contentRequests[i].contentRequest);
        contentRequests[i].take(data);
    }
    return ret;
}

void ICMPv6::freeContentRequest(ContentRequest* cr) {
    uint8_t i;
    for (i = 0;
            i < ICMP_MAX_CONTENTREQUESTS &&
            &(contentRequests[i].contentRequest) != cr;
            i++);
    if (i < ICMP_MAX_CONTENTREQUESTS) {
        LOG_DEBUG("ContentReq released");
        TAKE_MESSAGE(&(contentRequests[i].contentRequest));
        contentRequests[i].freeHolder();
    }
}

}
