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
 * Parent Class for all kinds of Distributed Algorithms
 * @author Bastian Weigelt
 */

#ifndef DISTALGO_H_
#define DISTALGO_H_

#include "DistAlgoData.h"
#include "DistAlgoHeader.h"
#include "DistAlgoAnalysis.h"
#include "TZTCAElement.h"


namespace cometos {

class DistAlgorithm{
public:
    DistAlgorithm()
        {

        };

    virtual void initialize(TZTCAElement *neighborhoodList) = 0;
    virtual uint8_t evaluateGuards() = 0;
    virtual bool execute(uint8_t activeGuard) = 0;
    virtual void finally() = 0;
    virtual void setCache(DataIndication& msg) = 0;
    virtual void saveLocalState() = 0;
    virtual void revertLocalState() = 0;

    virtual void outReportString();
    virtual void outAlgoAbbrev();

    virtual void addHeader(Airframe& msg);
    virtual bool checkMsgType(Airframe& msg);
    virtual uint8_t getAlgoType();

    void setLocalState(uint8_t newState);
    uint8_t getCacheSize();
    uint8_t getLocalState();
//    virtual DistAlgoData* getDataPtr() = 0;

protected:
    virtual uint8_t getIndexOf(node_t idIn) = 0;

//    DistAlgoData mNeighborStates[NEIGHBORLISTSIZE + STANDBYLISTSIZE];
    uint8_t mCacheSize;
    uint8_t mLocalState;
    bool mActive;

};
}

#endif /* DISTALGO_H_ */
