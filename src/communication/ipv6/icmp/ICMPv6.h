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

#ifndef ICMPV6_H_
#define ICMPV6_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "palLocalTime.h"
#include "SList.h"
#include "IPv6Datagram.h"
#include "logging.h"

/*MACROS---------------------------------------------------------------------*/

#define ICMP_MODULE_NAME            "icm"

const uint8_t ICMP_ECHO_MAX_HANDLERS =      6;
const uint8_t ICMP_MAX_LISTENERS =          6;
const uint8_t ICMP_MAX_CONTENTREQUESTS =    4;
const uint16_t ICMP_CHECK_TIMEOUT =         5000;

enum {
    ICMP_TYPE_DESTINATION_UNREACHABLE   = 1,
    ICMP_TYPE_PACKET_TOO_BIG            = 2,
    ICMP_TYPE_TIME_EXCEEDED             = 3,
    ICMP_TYPE_PARAMETER_PROBLEM         = 4,
    ICMP_TYPE_ECHO_REQUEST              = 128,
    ICMP_TYPE_ECHO_REPLY                = 129
};

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {


/**
 * baseEchoReceiver
 * Abstraction Class for Handlers of an ECHO REPLY
 */
class baseEchoReceiver {
public:
    virtual ~baseEchoReceiver() {
    }
    virtual void callHandler(uint16_t seqNum, bool success) const = 0;
};

/**
 * echoReceiver
 * Abstraction of C++ method pointers for an ECHO REPLY
 */
template <class C>
class echoReceiver: public baseEchoReceiver {
public:
    echoReceiver(C* owner, void (C::*handler)(uint16_t, bool)):
                owner(owner),
                handler(handler)
    {
    }
    void callHandler(uint16_t seqNum, bool success) const {
        (owner->*handler)(seqNum, success);
    }

protected:
    C*  owner;
    void (C::*handler)(uint16_t, bool);
};

class ICMPv6Listener {
public:
    virtual void icmpMessageReceived(const IPv6Address& src,
            const IPv6Address & dst,
            uint8_t type,
            uint8_t code,
            const uint16_t additionalValues[2],
            const uint8_t* data,
            uint16_t length) = 0;
    virtual ~ICMPv6Listener() {};
};

//class baseICMPListener {
//public:
//    virtual ~baseICMPListener() {}
//    virtual void callHandler(const IPv6Address& src,
//            const IPv6Address & dst,
//            uint8_t type,
//            uint8_t code,
//            const uint16_t additionalValues[2],
//            const uint8_t* data,
//            uint16_t length) const = 0;
//};
//
//template <class C>
//class ICMPListener: public baseICMPListener {
//public:
//    typedef void (C::*ICMPHandler)(const IPv6Address&, const IPv6Address & dst, uint8_t, uint8_t, const uint16_t[2], const uint8_t*, uint16_t);
//
//    ICMPListener(C* owner, ICMPHandler handler):
//                owner(owner),
//                handler(handler)
//    {}
//
//    void callHandler(const IPv6Address& src,
//            const IPv6Address & dst,
//            uint8_t type,
//            uint8_t code,
//            const uint16_t additionalValues[2],
//            const uint8_t* data,
//            uint16_t length) const {
//        (owner->*handler)(src, dst, type, code, additionalValues, data, length);
//    }
//
//protected:
//    C*  owner;
//    ICMPHandler handler;
//};

class ICMPv6: public cometos::Module {
public:
    ICMPv6(const char * service_name = NULL);
    ~ICMPv6() {};
    void initialize();
    void finish() {
    #ifdef OMNETPP
        cancel(&timeOutSchedule);
        for (uint8_t i = 0; i < ICMP_ECHO_MAX_HANDLERS; i++) {
            if (echoHandlers[i].handler != NULL) {
                delete echoHandlers[i].handler;
            }
        }
//        for (uint8_t i = 0; i < ICMP_MAX_LISTENERS; i++) {
//            if (listeners[i].handler != NULL) {
//                delete listeners[i].handler;
//            }
//        }
        for (uint8_t i = 0; i < ICMP_MAX_CONTENTREQUESTS; i++) {
            TAKE_MESSAGE(&(contentRequests[i].contentRequest));
            delete contentRequests[i].contentRequest.content;
        }
    #endif
    }

    void handleIPRequest(ContentRequest *cRequest);

    void handleIPResponse(ContentResponse *cResponse);

    void handleTimeout(cometos::Message *msg);

    template <class C>
    bool sendEchoDP(const IPv6Address& dstAddr,
            uint16_t seqNum,
            const uint8_t* data,
            uint16_t length,
            C* owner = NULL,
            void (C::*handler)(uint16_t, bool) = NULL)
    {
        ENTER_METHOD_SILENT();
        uint16_t id = palLocalTime_get() & 0xFFFF;
        uint16_t addData[2] = {id, seqNum};
        uint8_t i = ICMP_ECHO_MAX_HANDLERS;

        if (handler !=  (void (C::*)(uint16_t, bool)) NULL) {
            i = getNextFreeEchoHandler();
            if (i == ICMP_ECHO_MAX_HANDLERS) {
                return false;
            }
        }

        bool ret = sendMessage(dstAddr,
                ICMP_TYPE_ECHO_REQUEST,
                0,
                addData,
                data,
                length);

        if (ret && (handler != (void (C::*)(uint16_t, bool)) NULL)) {
            LOG_DEBUG("Ins Hndlr Func at Pos " << (uint16_t)i);
            echoHandlers[i].handler = new echoReceiver<C>(owner, handler);
            echoHandlers[i].id      = id;
            echoHandlers[i].seqNum  = seqNum;
            echoHandlers[i].t       = 0;
        }

        return ret;
    }

    template <class C>
    bool sendEchoBI(const IPv6Address& dstAddr,
            uint16_t seqNum,
            BufferInformation* bi = NULL,
            C* owner = NULL,
            void (C::*handler)(uint16_t, bool) = NULL) {
        ENTER_METHOD_SILENT();
        uint8_t i = ICMP_ECHO_MAX_HANDLERS;
        uint16_t id = palLocalTime_get() & 0xFFFF;

        if (handler !=  (void (C::*)(uint16_t, bool)) NULL) {
            i = getNextFreeEchoHandler();
            if (i == ICMP_ECHO_MAX_HANDLERS) {
                return false;
            }
        }

        bool ret = sendEcho(dstAddr, seqNum, id, bi);

        if (ret && (handler != (void (C::*)(uint16_t, bool)) NULL)) {
            LOG_DEBUG("Ins Hndlr Func at Pos " << (uint16_t)i);
            echoHandlers[i].handler = new echoReceiver<C>(owner, handler);
            echoHandlers[i].id      = id;
            echoHandlers[i].seqNum  = seqNum;
            echoHandlers[i].t       = 0;
        }

        return ret;
    }

    bool sendEcho(const IPv6Address& dstAddr,
            uint16_t seqNum,
            uint16_t id,
            BufferInformation* bi = NULL) {
        ENTER_METHOD_SILENT();
        uint16_t addData[2] = {id, seqNum};

        return sendMessage(dstAddr, ICMP_TYPE_ECHO_REQUEST, 0, addData, bi);
    }

//    template <class C>
//    int8_t registerListener(C* owner,
//            void (C::*handler)(const IPv6Address&, const IPv6Address &, uint8_t, uint8_t, const uint16_t[2], const uint8_t*, uint16_t),
//            int16_t type = -1) {
//        uint8_t i = getNextFreeListener();
//        if (i < ICMP_MAX_LISTENERS) {
//            listeners[i].handler = new ICMPListener<C>(owner, handler);
//            listeners[i].type    = type;
//            return i;
//        }
//        return -1;
//    }

    bool registerListener(ICMPv6Listener * listener, int16_t type);

//    bool deregisterListener(uint8_t number);

    bool deregisterListener(ICMPv6Listener * listener, int16_t type);

    bool sendErrorMessage(IPv6Datagram* datagram, uint8_t type, uint8_t code);

    bool sendMessage(const IPv6Address& dst,
            uint8_t type,
            uint8_t code,
            const uint16_t addData[2],
            const uint8_t* data,
            uint16_t length);

    bool sendMessage(const IPv6Address& dst,
            uint8_t type,
            uint8_t code,
            const uint16_t addData[2],
            BufferInformation* bi = NULL);

    cometos::InputGate<ContentRequest>     fromIP;
    cometos::OutputGate<ContentRequest>    toIP;

protected:
//    uint8_t getNextFreeListener();
    uint8_t getNextFreeEchoHandler();

    ContentRequest* getContentRequest(BufferInformation* data);
    void freeContentRequest(ContentRequest* cr);

    /**
     * Type for saving ECHO REQUESTS and the methods for the reply
     */
    struct echoRequestHandler_t {
        baseEchoReceiver*   handler;
        uint16_t            id;
        uint16_t            seqNum;
        uint8_t             t;
        echoRequestHandler_t() : handler(NULL), id(0), seqNum(0), t(0) {}
    } echoHandlers[ICMP_ECHO_MAX_HANDLERS];
//
//    struct icmpListenerHandler_t {
//        baseICMPListener*   handler;
//        int16_t             type;
//        icmpListenerHandler_t(): handler(NULL), type(0) {}
//    } listeners[ICMP_MAX_LISTENERS];

    struct icmpListenerHandler_t {
        icmpListenerHandler_t(ICMPv6Listener * listener = NULL, int16_t type = -1): listener(listener), type(type) {}
        bool operator==(const icmpListenerHandler_t & other) const {
            return this->listener == other.listener && this->type == other.type;
        }
        ICMPv6Listener * listener;
        int16_t        type;
    };

    cometos::StaticSList<icmpListenerHandler_t, ICMP_MAX_LISTENERS> listeners;

    cometos::Message        timeOutSchedule;

    contentRequestHolder_t  contentRequests[ICMP_MAX_CONTENTREQUESTS];

};



} /* namespace cometos_v6 */
#endif /* ICMPV6_H_ */
