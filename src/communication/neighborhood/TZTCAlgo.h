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
 * Neighborhoodlist management
 * aka. Topology Control Algorithm
 * @author Gerry Siegemund, Bastian Weigelt
 */

#ifndef TZTCALGO_H_
#define TZTCALGO_H_

#include "TZTCAElement.h"


namespace cometos {

class TZTCAlgo{
public:
    TZTCAlgo()
    {

    };

    void initialize(node_t ownId);
    void handle(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn);
    void updateAllQuality();
    node_t checkDeletion(uint8_t inIndex, timestamp_t curTime);
    void resetClusterId();
    bool isBidirOnNL(node_t neighborId);

    TZTCAElement neighborView[NEIGHBORLISTSIZE + STANDBYLISTSIZE];
    uint8_t pPotentialReplaceIndex[NEIGHBORLISTSIZE];
    node_t pClusterId;
    uint16_t pClusterDist;

private:
    void checkNewRoot(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn);
    void checkPromotion(uint8_t inIndex, TCPWYHeader inHeader);
    void checkReplacement(uint8_t inIndex, TCPWYHeader inHeader);
    uint8_t determineSymmetricReplace(uint8_t inIndex);
    uint8_t determineClusterReplace(uint8_t inIndex, uint8_t numReplace, node_t newNodeClusterId);
    uint8_t determineQualityReplace(uint8_t inIndex, uint8_t numReplace);
    void updateClusterId(uint8_t inIndex);
    uint8_t nodesOnNL();
    void addPotentialReplace(uint8_t index);
    void resetPotentialReplace();
    uint8_t getIndexOf(node_t idIn);
    uint8_t nextEmptyListSpot(void);

    node_t mOwnId;

};
}

#endif /* TCALGO_H_ */
