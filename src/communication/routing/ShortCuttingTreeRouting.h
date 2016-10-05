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

#ifndef SHORTCUTTINGTREEROUTING_H_
#define SHORTCUTTINGTREEROUTING_H_

#include "Routing.h"
#include "DuplicateFilter.h"
#include "Vector.h"


// FIXED_BEACON should ix some error in the way the state udpate is handled, however, activating this fix
// decreases the performance of the protocol
#define FIXED_BEACONING

#define PARENT_LIST_LENGTH  3


// probability (in %) that a nodes try to increase the parent index
#define INCREASE_PARENT_PROB    5


class ShortCuttingTreeRouting : public Routing {
public:
    ShortCuttingTreeRouting();

    void initialize();

    void finish();

    void handleRequest(cometos::DataRequest* msg);

    void handleIndication(cometos::DataIndication* pkt, cometos::NwkHeader& nwk);

    void handleBeacon(cometos::DataIndication* pkt);

    //void handleEstablish(DataIndication* pkt);

    void handleRequest(cometos::DataRequest* msg, cometos::NwkHeader& nwk);

    void handleResponse(cometos::DataResponse* response);

    virtual void sendBeacon(cometos::Message *timer);

    void slotTimeout(cometos::Message *timer);

    cometos::InputGate<cometos::DataIndication>  beaconIn;

    cometos::OutputGate<cometos::DataRequest> beaconOut;

    void sendNext();

#ifdef FIXED_BEACONING
    void ttlTimeout(cometos::Message *timer);
    cometos::Message *ttlTimer;
    uint8_t ttlCounter;
#endif


protected:

    typedef cometos::Vector<node_t,PARENT_LIST_LENGTH> parentList_t;

    uint16_t slotDuration;

    bool isSending;
    uint8_t hops;
    parentList_t parents;

    uint8_t nextHops;
    parentList_t nextParents;


    node_t currentParent;
    uint8_t currentShortCutIndex;


    uint8_t broadcastTxPower;

    bool isSink;

    uint16_t slotsToRun;

    std::list<cometos::DataRequest*> queue;

    uint16_t sendingBackoff;

#ifdef ROUTING_ENABLE_STATS
    uint16_t forwarded;
    uint16_t control;
    uint16_t numControlRecv;
    int parentChanged;

    uint16_t lossQueue;
    uint16_t lossTransmission;
    uint16_t noRoute;
#endif

};

#endif /* SHORTCUTTINGTREEROUTING_H_ */
