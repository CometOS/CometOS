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
#include "SPTAlgo.h"
#include "palId.h"
#include "NetworkTime.h"

namespace cometos {

void SPTAlgo::initialize(TZTCAElement *neighborhoodList) {
    DistAlgorithm::initialize(neighborhoodList);
    for(int i = 0; i < mCacheSize; i++){
        mSPTNeighborStates[i].pNeighbor = &(neighborhoodList[i]);
        mSPTNeighborStates[i].pState = DIST_ALGO_STATE_INIT;
        mSPTNeighborStates[i].pTreeEdge = SPT_EDGE_UNKNOWN;
        mSPTNeighborStates[i].pTreeDist = 0;
        mSPTNeighborStates[i].pTreeID = SPT_INVALID_ID;
    }
    mLocalState = DIST_ALGO_STATE_INIT;
    mTreeID = palId_id();
    mTreeDist = 0;
    mTargetIndex = SPT_INVALID_U8;
    mLastAction = SPT_GUARD_NONE;
}

uint8_t SPTAlgo::evaluateGuards() {
    uint8_t activeGuard = DIST_ALGO_GUARD_NONE;
    mTargetIndex = SPT_INVALID_U8;
    bool hasNoNeighbors = true;
    node_t lowestTreeID = mTreeID;
    uint8_t indexTarget = SPT_INVALID_U8;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                hasNoNeighbors = false;
                bool stop = false;
                // search through states whether that had been an event at a neighbor
                switch(mSPTNeighborStates[i].pState) {
                case SPT_STATE_CIRCLE:
                    stop = true;
                    activeGuard = SPT_GUARD_REMOVE;
                    indexTarget = i;
                    break;
                case SPT_STATE_ADDED:
                    stop = true;
                    activeGuard = SPT_GUARD_ADD;
                    indexTarget = i;
                    break;
                case SPT_STATE_REMOVED:
                    stop = true;
                    activeGuard = SPT_GUARD_REMOVE;
                    indexTarget = i;
                    break;
                default:
                    break;
                }
                // if there had been an event
                if(stop) {
                    break;
                } else {
                    // if a neighbor is not connected in the tree but has a lower treeID than this node
                    if(mSPTNeighborStates[i].pTreeEdge != SPT_EDGE_YES) {
                        if(mSPTNeighborStates[i].pTreeID < lowestTreeID) {
                            // make him target of adding
                            lowestTreeID = mSPTNeighborStates[i].pTreeID;
                            activeGuard = SPT_GUARD_ADD;
                            indexTarget = i;
                        } else {
                            // if both are already in the tree but still have UNKNOWN edges set them to NO
                            if(lowestTreeID == mTreeID) {
                                if(mSPTNeighborStates[i].pTreeEdge == SPT_EDGE_UNKNOWN) {
                                    if(mSPTNeighborStates[i].pTreeID == mTreeID) {
                                        activeGuard = SPT_GUARD_REMOVE;
                                        indexTarget = i;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(hasNoNeighbors) {
        activeGuard = SPT_GUARD_ALONE;
    } else if(activeGuard != DIST_ALGO_GUARD_NONE) {
        mTargetIndex = indexTarget;
    }
    mLastAction = activeGuard;
    return activeGuard;
}

bool SPTAlgo::execute(uint8_t activeGuard) {
    bool didExecute = true;
    switch(activeGuard) {
        case SPT_GUARD_ADD:
            if(mTargetIndex != SPT_INVALID_U8) {
                mSPTNeighborStates[mTargetIndex].pTreeEdge = SPT_EDGE_YES;
            }
            break;
        case SPT_GUARD_REMOVE:
            if(mTargetIndex != SPT_INVALID_U8) {
                mSPTNeighborStates[mTargetIndex].pTreeEdge = SPT_EDGE_NO;
            }
            break;
        case SPT_GUARD_ALONE:
            didExecute = false;
            break;
        default:
            didExecute = false;
            break;
    }
    return didExecute;
}

void SPTAlgo::finally() {
    resetTreeList();
    resetTreeId();
}

void SPTAlgo::setCache(DataIndication& msg) {
    SPTHeader header;
    msg.getAirframe() >> header;
    // if it is a SPT msg
    if(header.pMsgType == SPT_MSG_TYPE_DEFAULT) {
        node_t index = getIndexOf(msg.src);
        if(index != SPT_INVALID_ID) {
            uint8_t neighborsChoice = SPT_EDGE_UNKNOWN;
            // searches header of neighbor for own entry
            for(uint8_t i = 0; i < sizeof(header.pNeighbors); i++) {
                if(header.pNeighbors[i] == palId_id()) {
                    neighborsChoice = header.pEdges[i];
                    break;
                }
            }
            // if it is the same choice as this node's
            if(neighborsChoice == mSPTNeighborStates[index].pTreeEdge) {
                mSPTNeighborStates[index].pState = SPT_STATE_OK;
                if(neighborsChoice == SPT_EDGE_YES) {
                    // if it is a tree ID update
                    if(mSPTNeighborStates[index].pTreeID != header.pTreeId) {
                        // check whether neighbor has a different route to the root than this node
                        if(header.pTreeId == mTreeID && header.pTreeDist < (mTreeDist + 1)) {
                            mSPTNeighborStates[index].pState = SPT_STATE_CIRCLE;
                        }
                    }
                    for(uint8_t i = 0; i < mCacheSize; i++) {
                        if(i != index) {
                            // if node has to neighbors joined the tree with the same distance
                            // that is smaller than this nodes distance => circle
                            if (mSPTNeighborStates[i].pTreeEdge == SPT_EDGE_YES) {
                                if(mSPTNeighborStates[i].pTreeID == header.pTreeId) {
                                    if(header.pTreeDist < mTreeDist) {
                                        if(mSPTNeighborStates[i].pTreeDist == header.pTreeDist) {
                                            mSPTNeighborStates[index].pState = SPT_STATE_CIRCLE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else { // not the same choice
                if(neighborsChoice == SPT_EDGE_NO) {
                    mSPTNeighborStates[index].pState = SPT_STATE_REMOVED;
                } else if(neighborsChoice == SPT_EDGE_YES) {
                    if(header.pTreeId != mTreeID) {
                        mSPTNeighborStates[index].pState = SPT_STATE_ADDED;
                    }
                }
            }
            //update his tree cluster values
            mSPTNeighborStates[index].pTreeID = header.pTreeId;
            mSPTNeighborStates[index].pTreeDist = header.pTreeDist;
        }
    }
}

void SPTAlgo::saveLocalState() {
    if(mTargetIndex < SPT_INVALID_U8) {
        mBackupTargetIndex = mTargetIndex;
        mBackupTreeEdge = mSPTNeighborStates[mTargetIndex].pTreeEdge;
    }
}

void SPTAlgo::revertLocalState() {
    if(mBackupTargetIndex < SPT_INVALID_U8) {
        mSPTNeighborStates[mBackupTargetIndex].pTreeEdge = mBackupTreeEdge;
        resetTreeId();
    }
}

void SPTAlgo::outReportString() {
    getCout() << "trID=" << (int) mTreeID << ";trDD=" << (int) mTreeDist;
    getCout() << ";cache={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                getCout() << "(" << (int)mSPTNeighborStates[i].pNeighbor->id << "," << (int)mSPTNeighborStates[i].pTreeEdge << "," << (int)mSPTNeighborStates[i].pTreeID << "," << (int)mSPTNeighborStates[i].pTreeDist << ")";
                atLeastOne = true;
            }
        }
    }
    getCout() << "}";
}

void SPTAlgo::outAlgoAbbrev() {
    getCout() << "SPT";
}

void SPTAlgo::addHeader(Airframe& msg) {
    // set target ID
    node_t target = SPT_INVALID_ID;
    uint8_t neighborCount = 0;
    if(mTargetIndex != SPT_INVALID_U8) {
        target = mSPTNeighborStates[mTargetIndex].pNeighbor->id;
    }
    // set header and add it to frame
    SPTHeader header(SPT_MSG_TYPE_DEFAULT, target, mTreeID, mTreeDist);
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            // can only be NEIGHBORLISTSIZE nodes at max
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                header.pNeighbors[neighborCount] = mSPTNeighborStates[i].pNeighbor->id;
                header.pEdges[neighborCount] = mSPTNeighborStates[i].pTreeEdge;
                neighborCount++;
            }
        }
    }
    msg << header;
}

uint8_t SPTAlgo::getAlgoType() {
    return DIST_ALGO_TYPE_SPT;
}

void SPTAlgo::resetTreeId() {
    node_t smallestTreeId = palId_id();
    uint16_t smallestTreeDist = 0;
    // iterate through all neighbors
    for(uint8_t i = 0; i < mCacheSize; i++) {
        // if neighbor is there and has bidirectional connection
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                // if edge to him is tree edge
                if(mSPTNeighborStates[i].pTreeEdge == SPT_EDGE_YES) {
                    if(mSPTNeighborStates[i].pTreeID == smallestTreeId) {
                        if(mSPTNeighborStates[i].pTreeDist < smallestTreeDist) {
                            smallestTreeDist = mSPTNeighborStates[i].pTreeDist;
                        }
                    } else if(mSPTNeighborStates[i].pTreeID < smallestTreeId) {
                        smallestTreeId = mSPTNeighborStates[i].pTreeID;
                        smallestTreeDist = mSPTNeighborStates[i].pTreeDist;
                    }
                }
            }
        }
    }
    mTreeID = smallestTreeId;
    mTreeDist = smallestTreeDist;
    if((mTreeDist < SPT_MAX_DISTANCE) && (mTreeID != palId_id())) {
        mTreeDist++;
    } else {
        mTreeID = palId_id();
        mTreeDist = 0;
    }
}

void SPTAlgo::resetTreeList(){
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pTreeEdge != SPT_EDGE_UNKNOWN) {
            if((mSPTNeighborStates[i].pNeighbor->id == SPT_INVALID_ID) || (!mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink())) {
                mSPTNeighborStates[i].remove();
            }
        }
    }
}

uint8_t SPTAlgo::getIndexOf(node_t inId){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i < mCacheSize; i++){
        if(mSPTNeighborStates[i].pNeighbor->id == inId) {
            return i;
        }
    }
    return index;
}

SPTData* SPTAlgo::getDataPtr() {
    return mSPTNeighborStates;
}

node_t SPTAlgo::getTreeID() {
    return mTreeID;
}

uint16_t SPTAlgo::getTreeDist() {
    return mTreeDist;
}

}
