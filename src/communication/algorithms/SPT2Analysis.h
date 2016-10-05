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
 * @author Bastian Weigelt
 */

#ifndef SPT2ANALYSIS_H_
#define SPT2ANALYSIS_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Layer.h"
#include "TZTypes.h"
#include "TZTCAElement.h"
#include "DistAlgoAnalysis.h"
#include "SPT2Data.h"
#include "SPT2Header.h"
#include "SPT2Algo.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class SPT2Analysis : public DistAlgoAnalysis{
public:
    SPT2Analysis() : DistAlgoAnalysis() {
        mAlgo = nullptr;
        mTreeID = SPT_INVALID_ID;
    };

    SPT2Analysis(uint8_t initState, TZTCAlgo* tca) : DistAlgoAnalysis(initState, tca, 0) {
        mAlgo = nullptr;
        mTreeID = SPT_INVALID_ID;
    };

    SPT2Analysis(uint8_t initState, TZTCAlgo* tca, SPT2Algo* inAlgo)
    : DistAlgoAnalysis(initState,tca, inAlgo->getCacheSize()) {
        mAlgo = inAlgo;
        mTreeID = mAlgo->getTreeID();
    };

    virtual void countState(uint8_t inCurrentState, timestamp_t inTime);

    virtual void printAll();
    virtual void printNeighbor();
    virtual void printState();

    virtual uint8_t getAnalyserType();

    void initialize(SPT2Algo* inAlgo, uint8_t inCacheSize);

protected:

    SPT2Algo* mAlgo;

    node_t mTreeID;

};

} // namespace cometos

#endif /* SPT2ANALYSIS_H_ */
