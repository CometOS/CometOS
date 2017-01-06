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
 * @author Gerry Siegemund
 */
#include "TZTCAElement.h"
#include "TZTypes.h"
#include "palId.h"


namespace cometos {

void TZTCAElement::updateQuality(){
    /*
     * TODO: Reimplement!!!!
     */
    // only increases quality if there has been at least one msg from neighbor within last time frame
    //          else it decreases the quality
    // for calculations see paper Renner,'Prediction Accuracy of Link-Quality Estimators'
    // calc: Q_st = alpha * Q_st + (1 - alpha) * receivedMsg
    uint16_t qualityIncrease = (Q_SCALE - Q_ALPHA) * receivedSinceLastUpdate;
    uint32_t temp = (Q_ALPHA * qualityST) / Q_SCALE;
    qualityST = (uint16_t)temp + qualityIncrease;
    // calc: Q_lt = beta * Q_lt + (1 - beta) * Q_st
    temp = (Q_BETA * qualityLT) / Q_SCALE;
    qualityLT = (uint16_t)temp;
    temp = ((Q_SCALE - Q_BETA) * qualityST) / Q_SCALE;
    qualityLT += temp;

    receivedSinceLastUpdate = 0;
}

void TZTCAElement::updateQuality(uint8_t seqNumIn, node_t ccIDIn, node_t ccDistIn, timestamp_t timeIn){
    ASSERT(false); // might not work as expected, use updateQuality() instead

    bool isLastCalc = false;
    // check whether received sequence number is next after latest
    if(seqNumIn == (uint8_t)(lastSeqNum + (uint8_t)1)) {
        isLastCalc = true;
    }
    do {
        // see above
        if(seqNumIn == (uint8_t)(lastSeqNum + (uint8_t)1)) {
            isLastCalc = true;
        }
        // for calculations see paper Renner,'Prediction Accuracy of Link-Quality Estimators'
        // calc: Q_st = alpha * Q_st + (1 - alpha) * isLastCalc
        uint16_t qualityIncrease = (Q_SCALE - Q_ALPHA) * isLastCalc;
        uint32_t temp = (Q_ALPHA * qualityST) / Q_SCALE;
        qualityST = (uint16_t)temp + qualityIncrease;
        // calc: Q_lt = beta * Q_lt + (1 - beta) * Q_st
        temp = (Q_BETA * qualityLT) / Q_SCALE;
        qualityLT = (uint16_t)temp;
        temp = ((Q_SCALE - Q_BETA) * qualityST) / Q_SCALE;
        qualityLT += temp;
        lastSeqNum++;
    } while(!isLastCalc);
    // saving current values in list
    lastSeqNum = seqNumIn;
    timestamp = timeIn;
}

void TZTCAElement::checkCoherence(){ //curTime

}

bool TZTCAElement::hasBidirectionalLink() {
    //// checks whether self is part of neighbor-list of neighbor
    bool result = false;
    if(onNL) {
        result = hasLinkToThis();
    }
    return result;
}

bool TZTCAElement::hasLinkToThis() {
    bool result = false;
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
        if(neighbors[i] == palId_id()){
            result = true;
        }
    }
    return result;
}

void TZTCAElement::add(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn){
    id = idIn;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE; i++){
        neighbors[i] = headerIn.neighbor[i];
    }
    lastSeqNum = headerIn.seqNum;
    receivedSinceLastUpdate = 1; // the add is only called for a received packet
    ccID = headerIn.ccID;
    ccDist = headerIn.ccDist;
    qualityST = Q_INIT; // we start @ 0.3, i.e., 3000"/10000" to make the start-up phase a little shorter
    qualityLT = Q_INIT;
    timestamp = timeIn;
    onNL = false;
    tries = 0;

#ifdef LOCATION_IN_TCPWYHeader
    coordinates = headerIn.coordinates;
#endif
}

void TZTCAElement::update(TCPWYHeader headerIn, timestamp_t timeIn){
    for(uint8_t i=0; i<NEIGHBORLISTSIZE; i++){
        neighbors[i] = headerIn.neighbor[i];
    }
    ccID = headerIn.ccID;
    ccDist = headerIn.ccDist;
    lastSeqNum = headerIn.seqNum;
    timestamp = timeIn;

#ifdef LOCATION_IN_TCPWYHeader
    coordinates = headerIn.coordinates;
#endif
}

void TZTCAElement::join() {
    onNL = true;
    tries = 0;
}

void TZTCAElement::remove(){
    id = TZ_INVALID_ID;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE; i++){
        neighbors[i] = TZ_INVALID_ID;
    }
    lastSeqNum = 0;
    receivedSinceLastUpdate = 0;
    ccID = TZ_INVALID_ID;
    ccDist = TZ_INVALID_ID;
    qualityST = 0; // we start @ 0.3 to make the start-up phase a little shorter
    qualityLT = 0; // we start @ 0.3 to make the start-up phase a little shorter
    timestamp = 0;
    onNL = false;
    tries = 0;
}

}
