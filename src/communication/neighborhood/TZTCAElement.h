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
 * I DO NOT HAVE A TIMER OF MY OWN!
 * THERE MIGHT BE A "CLEANUP FUNCTION" WHICH IS CALLED FROM THE OUTSIDE
 * (IF NOT I CAME UP WITH A BETTER (OR WORSE ;) ) DESIGN)
 *
 * LEST MANAGEMENT IS DONE ELSEWHERE!
 * @author Gerry Siegemund
 */

#ifndef TZTCAELEMENT_H_
#define TZTCAELEMENT_H_

#include "TZTypes.h"
#include "cometos.h"
#include "TCPWYHeader.h"


namespace cometos {

class TZTCAElement{
public:
    TZTCAElement()
    {

    };

    void updateQuality();
    void updateQuality(uint8_t seqNumIn, node_t ccIDIn, node_t ccDistIn, timestamp_t timeIn);
    void checkCoherence(void); //curTime
    bool hasBidirectionalLink();
    bool hasLinkToThis();
    void add(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn);
    void update(TCPWYHeader headerIn, timestamp_t timeIn);
    void join();
    void remove();

    node_t id;
    node_t neighbors[NEIGHBORLISTSIZE];
    uint8_t lastSeqNum;
    uint16_t receivedSinceLastUpdate;
    node_t ccID;
    uint16_t ccDist;
    uint16_t qualityST;
    uint16_t qualityLT;
    timestamp_t timestamp;
    //considered a neighbor, if false -> can still become a neighbor but it is currently evaluated
    bool onNL;
    uint8_t tries;

};
}

#endif /* TZTCAELEMENT_H_ */
