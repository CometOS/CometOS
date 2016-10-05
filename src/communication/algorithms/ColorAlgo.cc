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
#include "ColorAlgo.h"
#include "palId.h"
#include "NetworkTime.h"

namespace cometos {

void ColorAlgo::initialize(TZTCAElement *neighborhoodList) {
    DistAlgorithm::initialize(neighborhoodList);
    for(int i = 0; i < mCacheSize; i++){
        mNeighborStates[i].pNeighbor = &(neighborhoodList[i]);
        mNeighborStates[i].pState = COLOR_INVALID_U8;
    }
    for(uint8_t i = 0; i < COLOR_MAX; i++) {
        mColorAvailable[i] = true;
    }
    // stands for own color
    mLocalState = COLOR_INVALID_U8;
    mBackupColor = COLOR_INVALID_U8;
}

uint8_t ColorAlgo::evaluateGuards() {
    uint8_t activeGuard = DIST_ALGO_GUARD_NONE;
    resetColorAvailable();
    bool hasNoNeighbors = true;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mNeighborStates[i].pNeighbor->id != COLOR_INVALID_ID) {
            if(mNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                hasNoNeighbors = false;
                if(mNeighborStates[i].pState != COLOR_INVALID_U8) {
                    mColorAvailable[mNeighborStates[i].pState] = false;
                }
            }
        }
    }
    if(hasNoNeighbors) {
        activeGuard = DIST_ALGO_GUARD_ALONE;
    } else if(mLocalState == COLOR_INVALID_U8) {
        activeGuard = COLOR_GUARD_CHANGE;
    } else if(getMinColor() != mLocalState) {
        activeGuard = COLOR_GUARD_CHANGE;
    }
    return activeGuard;
}

bool ColorAlgo::execute(uint8_t activeGuard) {
    bool didExecute = false;
    switch(activeGuard) {
        case COLOR_GUARD_CHANGE:
            mLocalState = getMinColor();
            didExecute = true;
            break;
        case DIST_ALGO_GUARD_ALONE:
            break;
        default:
            break;
    }
    return didExecute;
}

void ColorAlgo::finally() {

}

void ColorAlgo::setCache(DataIndication& msg) {
    ColorHeader header;
    msg.getAirframe() >> header;
    // if it is a Color msg
    if(header.pMsgType == COLOR_MSG_TYPE_DEFAULT) {
        node_t index = getIndexOf(msg.src);
        if(index != COLOR_INVALID_ID) {
            mNeighborStates[index].pState = header.pState;
        }
    }
}

void ColorAlgo::saveLocalState() {
    mBackupColor = mLocalState;
}

void ColorAlgo::revertLocalState() {
    mLocalState = mBackupColor;
    mBackupColor = COLOR_INVALID_U8;
}

void ColorAlgo::outReportString() {
    getCout() << "color=" << (int) mLocalState << ";cache={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mNeighborStates[i].pNeighbor->id != COLOR_INVALID_ID) {
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

void ColorAlgo::outAlgoAbbrev() {
    getCout() << "COL";
}

void ColorAlgo::addHeader(Airframe& frame) {
    // set header and add it to frame
    ColorHeader header(COLOR_MSG_TYPE_DEFAULT, mLocalState);
    frame << header;
}

bool ColorAlgo::checkMsgType(Airframe& frame) {
    // checks whether the msg is of type MIS
    bool result = false;
    uint8_t msgType = 0;
    frame >> msgType;

    if(msgType == COLOR_MSG_TYPE_DEFAULT) {
        result = true;
    }
    frame << msgType;
    return result;
}

uint8_t ColorAlgo::getAlgoType() {
    return DIST_ALGO_TYPE_COLOR;
}

uint8_t ColorAlgo::getMinColor() {
    uint8_t minColor = 0;
    while(!mColorAvailable[minColor]) {
        minColor++;
        if(minColor >= COLOR_MAX) {
            break;
        }
    }
    return minColor;
}

void ColorAlgo::resetColorAvailable() {
    for(uint8_t i = 0; i < COLOR_MAX; i++) {
        mColorAvailable[i] = true;
    }
}

uint8_t ColorAlgo::getIndexOf(node_t inId){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i < mCacheSize; i++){
        if(mNeighborStates[i].pNeighbor->id == inId) {
            return i;
        }
    }
    return index;
}

ColorData* ColorAlgo::getDataPtr() {
    return mNeighborStates;
}

}
