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
#include "SPTAnalysis.h"
#include "palId.h"
#include "NetworkTime.h"

namespace cometos {

void SPTAnalysis::countState(uint8_t inState, timestamp_t inTime){
    if(mIsActive) {
        node_t treeID = mAlgo->getTreeID();
        if(mTreeID == treeID) {
            mCountCorrectState++;
            if(mCountCorrectState >= THRESHOLD_COUNT_CONVERGENCE) {
                mIsSteady = true;
            }
        } else {
            mTreeID = treeID;
            mConvTime = inTime;
            mIsSteady = false;
        }
        if(!mHasNeighbor) {
            for(uint8_t i = 0; i < mCacheSize; i++) {
                if(mAlgo->getDataPtr()[i].pNeighbor->hasBidirectionalLink()) {
                    mHasNeighbor = true;
                    mNeighborTime = inTime;
                    break;
                }
            }
        }
    }
}

void SPTAnalysis::printNeighbor() {
    getCout() << (long)NetworkTime::get() << ";TCA_TIME;node[" << palId_id() << "];clID=" << mTCA->pClusterId << ";clDD=" << mTCA->pClusterDist  << ";neighbor={";
    bool atLeastOne = false;
    for(int i = 0; i < mCacheSize; i++) {
        if(mAlgo->getDataPtr()[i].pNeighbor->hasBidirectionalLink()) {
            if(atLeastOne) {
                getCout() << ",";
            }
            //getCout() << "(" << tca.neighborView[i].id << "," << tca.neighborView[i].qualityLT << ")";
            getCout() << mAlgo->getDataPtr()[i].pNeighbor->id;
            atLeastOne = true;
        }
    }
    atLeastOne = false;
    getCout() << "};nl={";
    for(int i = 0; i < mCacheSize; i++) {
        if(mAlgo->getDataPtr()[i].pNeighbor->onNL) {
            if(!mAlgo->getDataPtr()[i].pNeighbor->hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                //getCout() << "(" << tca.neighborView[i].id << "," << tca.neighborView[i].qualityLT << ")";
                getCout() << mAlgo->getDataPtr()[i].pNeighbor->id;
                atLeastOne = true;
            }
        }
    }
    getCout() << "}!\n";
}

void SPTAnalysis::printState() {
    getCout() << (long)NetworkTime::get() << ";SPT_TIME;node[" << palId_id() << "];trID=" << (int) mAlgo->getTreeID() << ";trDD=" << (int) mAlgo->getTreeDist() << ";cache={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mAlgo->getDataPtr()[i].pNeighbor->id != SPT_INVALID_ID) {
            if(mAlgo->getDataPtr()[i].pNeighbor->hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                getCout() << "(" << (int)mAlgo->getDataPtr()[i].pNeighbor->id << "," << (int)mAlgo->getDataPtr()[i].pTreeEdge << "," << (int)mAlgo->getDataPtr()[i].pTreeID << "," << (int)mAlgo->getDataPtr()[i].pTreeDist << ")";
                atLeastOne = true;
            }
        }
    }
    getCout() << "}!\n";

}

uint8_t SPTAnalysis::getAnalyserType() {
    return DIST_ALGO_TYPE_SPT;
}

void SPTAnalysis::initialize(SPTAlgo* inAlgo, uint8_t inCacheSize) {
    mAlgo = inAlgo;
    mCacheSize = inCacheSize;
    mIsActive = true;
}

} /* namespace cometos */
