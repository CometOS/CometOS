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
#include "MISAlgo.h"
#include "palId.h"
#include "NetworkTime.h"

//#define OUTPUT_WEIGELT


namespace cometos {

void MISAlgo::initialize(TZTCAElement *neighborhoodList) {
    DistAlgorithm::initialize(neighborhoodList);
    for(uint8_t i = 0; i < mCacheSize; i++) {
        mNeighborStates[i].pNeighbor = &(neighborhoodList[i]);
        mNeighborStates[i].pState = DIST_ALGO_STATE_INIT;
    }
    mLocalState = DIST_ALGO_STATE_INIT;
    mBackupLocalState = mLocalState;
}

uint8_t MISAlgo::evaluateGuards() {
    uint8_t activeGuard = MIS_GUARD_NONE;
    bool hasNoNeighbors = true;
    // distinguishes whether this node is in the MIS or not
    if(mLocalState == MIS_STATE_IN) {
        for(uint8_t i = 0; i < mCacheSize; i++) {
            // checks for all active neighbors with bidirectional link
            if(mNeighborStates[i].pNeighbor->id != MIS_INVALID_ID) {
                if(mNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                    hasNoNeighbors = false;
                    // when they are in the MIS this node leaves
                    if(mNeighborStates[i].pState == MIS_STATE_IN) {
                        activeGuard = MIS_GUARD_LEAVE;
                    }
                }
            }
        }
        // if node has no neighbors he leaves MIS
        if(hasNoNeighbors) {
            activeGuard = DIST_ALGO_GUARD_ALONE;
        }
    } else { // not in MIS
        activeGuard = MIS_GUARD_JOIN;
        for(uint8_t i = 0; i < mCacheSize; i++) {
            // checks for all active neighbors with bidirectional link
            if(mNeighborStates[i].pNeighbor->id != MIS_INVALID_ID) {
                if(mNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                    hasNoNeighbors = false;
                    // when they are in the MIS this node leaves
                    if(mNeighborStates[i].pState == MIS_STATE_IN) {
                        if(mLocalState == DIST_ALGO_STATE_INIT) {
                            activeGuard = MIS_GUARD_LEAVE;
                        } else {
                            activeGuard = MIS_GUARD_NONE;
                        }
                        break;
                    }
                }
            }
        }
        // if no has no neighbors he does not join MIS
        if(hasNoNeighbors) {
            activeGuard = DIST_ALGO_GUARD_ALONE;
        }
    }
    return activeGuard;
}

bool MISAlgo::execute(uint8_t activeGuard) {
    bool didExecute = true;
    switch(activeGuard) {
        case MIS_GUARD_JOIN:
            mLocalState = MIS_STATE_IN;
            break;
        case MIS_GUARD_LEAVE:
            mLocalState = MIS_STATE_OUT;
            break;
        case DIST_ALGO_GUARD_ALONE:
            didExecute = false;
            break;
        default:
            didExecute = false;
            break;
    }
    return didExecute;
}

void MISAlgo::finally() {

}

void MISAlgo::setCache(DataIndication& msg) {
    MISHeader header;
    msg.getAirframe() >> header;
    if(header.pMsgType == MIS_MSG_TYPE_DEFAULT) {
        node_t index = getIndexOf(msg.src);
        if(index != MIS_INVALID_ID) {
            mNeighborStates[index].pState = header.pState;
        }
    }
}

void MISAlgo::saveLocalState() {
    mBackupLocalState = mLocalState;
}

void MISAlgo::revertLocalState() {
    mLocalState = mBackupLocalState;
}

void MISAlgo::outReportString() {
    getCout() << "state=" << (int)mLocalState << ";";
    getCout() << "cache={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mNeighborStates[i].pNeighbor->id != MIS_INVALID_ID) {
            if(mNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                getCout() << "(" << (int)mNeighborStates[i].pNeighbor->id << "," << (int)mNeighborStates[i].pState << ")";
                atLeastOne = true;
            }
        }
    }
    getCout() << "}";
}

void MISAlgo::outAlgoAbbrev() {
    getCout() << "MIS";
}

void MISAlgo::addHeader(Airframe& msg) {
    MISHeader header(MIS_MSG_TYPE_DEFAULT, mLocalState);
    msg << header;
}

uint8_t MISAlgo::getAlgoType() {
    return DIST_ALGO_TYPE_MIS;
}

MISData* MISAlgo::getDataPtr() {
    return mNeighborStates;
}

uint8_t MISAlgo::getIndexOf(node_t inId){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i < mCacheSize; i++){
        if(mNeighborStates[i].pNeighbor->id == inId) {
            return i;
        }
    }
    return index;
}


}
