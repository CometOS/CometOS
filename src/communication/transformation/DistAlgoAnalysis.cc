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
#include "DistAlgoAnalysis.h"
#include "palId.h"
#include "NetworkTime.h"

namespace cometos {

void DistAlgoAnalysis::countMessage(bool inSuccess){
    if(mIsActive) {
        if(inSuccess) {
            mCountMsgSuccess++;
        } else {
            mCountMsgFail++;
        }
    }
}

void DistAlgoAnalysis::countReceived() {
    mCountReceived++;
}

void DistAlgoAnalysis::countState(uint8_t inState, timestamp_t inTime){
    if(mIsActive) {
        // if node hasn't reached steady state
        if(!mIsSteady) {
            if(inState == mCurrentState) {
                // only count if it is not the initial state
                if(mCurrentState != DIST_ALGO_STATE_INIT) {
                    mCountCorrectState++;
                    if(mCountCorrectState >= THRESHOLD_COUNT_CONVERGENCE) {
                        mIsSteady = true;
                    }
                }
            } else {
                mCurrentState = inState;
                mConvTime = inTime;
                mIsSteady = false;
                mCountCorrectState = 0;
            }
        }
    }
}

void DistAlgoAnalysis::printAll() {
    printAnalysis();
    printNeighbor();
    printState();
}

void DistAlgoAnalysis::printAnalysis() {
    getCout() << (long)NetworkTime::get() << ";ANA_T;v[" << (int)palId_id()
            << "];ts=" << (long)mConvTime << ";tnl=" << (long)mNeighborTime
            << ";m1=" << (int)mCountMsgSuccess << ";m0=" << (int)mCountMsgFail
            << ";mr=" << (int)mCountReceived << "!\n";
    mCountMsgSuccess = 0;
    mCountMsgFail = 0;
    mCountReceived = 0;
}

void DistAlgoAnalysis::printNeighbor() {
    getCout() << (long)NetworkTime::get() << ";TCA_T;v[" << palId_id() << "];cI=" << mTCA->pClusterId << ";cD=" << mTCA->pClusterDist  << ";n={";
    bool atLeastOne = false;
    for(int i = 0; i < mCacheSize; i++) {
        if(mTCA->neighborView[i].hasBidirectionalLink()) {
            if(atLeastOne) {
                getCout() << ",";
            }
            //getCout() << "(" << tca.neighborView[i].id << "," << tca.neighborView[i].qualityLT << ")";
            getCout() << mTCA->neighborView[i].id;
            atLeastOne = true;
        }
    }
/*
    atLeastOne = false;
    getCout() << "};nl={";
    for(int i = 0; i < mCacheSize; i++) {
        if(mTCA->neighborView[i].onNL) {
            if(!mTCA->neighborView[i].hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                //getCout() << "(" << tca.neighborView[i].id << "," << tca.neighborView[i].qualityLT << ")";
                getCout() << mTCA->neighborView[i].id;
                atLeastOne = true;
            }
        }
    }
*/
    getCout() << "}!\n";
}

void DistAlgoAnalysis::printState() {
    getCout() << (long)NetworkTime::get() << ";DIA_T;v[" << palId_id() << "];";
    getCout() << "s=" << (int)mCurrentState << "!\n";
}

uint8_t DistAlgoAnalysis::getAnalyserType() {
    return DIST_ALGO_TYPE_NONE;
}

void DistAlgoAnalysis::activate() {
    mIsActive = true;
}

void DistAlgoAnalysis::deactivate() {
    mIsActive = false;
}

}
