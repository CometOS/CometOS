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
#include "SPT2Algo.h"
#include "palId.h"
#include "NetworkTime.h"

#define SPT_MINIMIZE_COUNT 5

namespace cometos {

void SPT2Algo::initialize(TZTCAElement *neighborhoodList) {
    DistAlgorithm::initialize(neighborhoodList);
    for(int i = 0; i < mCacheSize; i++){
        mSPTNeighborStates[i].pNeighbor = &(neighborhoodList[i]);
        mSPTNeighborStates[i].pState = DIST_ALGO_STATE_INIT;
        mSPTNeighborStates[i].pTreeState = SPT_TREE_NONE;
    }
    mLocalState = SPT_STATE_INIT;
    mTreeID = palId_id();
    mTreeDist = 0;
    mMinimizeCount = 0;
    mIsRoot = false;
    mMsgAction = SPT_GUARD_NONE;
}

uint8_t SPT2Algo::evaluateGuards() {
    uint8_t activeGuard = DIST_ALGO_GUARD_NONE;
    bool hasNoNeighbors = true;
    if(mLocalState != SPT_STATE_INIT) {
        if(mIsRoot) {
            mIsRoot = true;
        }
        for(uint8_t i = 0; i < mCacheSize; i++) {
            if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
                if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                    hasNoNeighbors = false;
                    // if node has no father yet
                    if(mLocalState == SPT_STATE_OUT) {
                        if(mSPTNeighborStates[i].pState == SPT_STATE_IN) {
                            activeGuard = SPT_GUARD_JOIN;
                            break;
                        }
                    } else { // if state is in
                        if(mSPTNeighborStates[i].pState == SPT_STATE_INIT) {
                            activeGuard = SPT_GUARD_DISCOVER;
                            break;
                        }
                        // if node is in its own tree but not root, set alone
                        if(mTreeID == palId_id() && !mIsRoot) {
                            activeGuard = DIST_ALGO_GUARD_ALONE;
                            break;
                        } else {
                            activeGuard = SPT_GUARD_MINIMIZE;
                        }
                    }
                }
            }
        }
    }
    // only do something if node is not root
    if(mIsRoot) {
        if(activeGuard != SPT_GUARD_DISCOVER) {
            activeGuard = DIST_ALGO_GUARD_ALONE;
        }
    }
    if(hasNoNeighbors) {
        activeGuard = DIST_ALGO_GUARD_ALONE;
    }
    return activeGuard;
}

bool SPT2Algo::execute(uint8_t activeGuard) {
    bool didExecute = false;
    uint8_t indexSmallest = SPT_INVALID_U8;
    uint16_t smallestDist = SPT_INVALID_U16;
    node_t smallestTreeID = SPT_INVALID_ID;
    switch(activeGuard) {
        case SPT_GUARD_JOIN:
            // searches all neighbors for one with smallest distance to root
            for(uint8_t i = 0; i < mCacheSize; i++) {
                if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
                    if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                        // if neighbor is in tree
                        if(mSPTNeighborStates[i].pState == SPT_STATE_IN) {
                            // if neighbor has smallest known tree ID -> closest tree to root
                            if(mSPTNeighborStates[i].pTreeID < smallestTreeID) {
                                indexSmallest = i;
                                smallestTreeID = mSPTNeighborStates[i].pTreeID;
                                smallestDist = mSPTNeighborStates[i].pTreeDist;
                            } else if(mSPTNeighborStates[i].pTreeID == smallestTreeID) {
                                // smaller dist than node has registered
                                if(mSPTNeighborStates[i].pTreeDist < smallestDist) {
                                    indexSmallest = i;
                                    smallestDist = mSPTNeighborStates[i].pTreeDist;
                                }
                            }
                        }
                    }
                }
            }
            // setting new tree values
            if(indexSmallest != SPT_INVALID_U8) {
                mFatherID = mSPTNeighborStates[indexSmallest].pNeighbor->id;
                mTreeID = smallestTreeID;
                mTreeDist = smallestDist + 1;
                mMsgAction = SPT_GUARD_JOIN;
                mLocalState = SPT_STATE_IN;
                didExecute = true;
            }
            break;
        case SPT_GUARD_DISCOVER:
            mMsgAction = SPT_GUARD_DISCOVER;
            didExecute = true;
            break;
        case SPT_GUARD_MINIMIZE:
            // only minimize every n-th round to reduce overall actions
            if(mMinimizeCount >= SPT_MINIMIZE_COUNT) {
                uint8_t indexFather = SPT_INVALID_U8;
                mMinimizeCount = 0;
                smallestDist = mTreeDist - 1;
                // searches all neighbors for one with smallest distance to root than self
                for(uint8_t i = 0; i < mCacheSize; i++) {
                    if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
                        if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                            // if neighbor is in tree
                            if(mSPTNeighborStates[i].pState == SPT_STATE_IN) {
                                // if neighbor has smallest known tree ID -> closest tree to root
                                if(mSPTNeighborStates[i].pTreeID < smallestTreeID) {
                                    indexSmallest = i;
                                    smallestTreeID = mSPTNeighborStates[i].pTreeID;
                                    smallestDist = mSPTNeighborStates[i].pTreeDist;
                                } else if(mSPTNeighborStates[i].pTreeID == smallestTreeID) {
                                    // smaller dist than node has registered
                                    if(mSPTNeighborStates[i].pTreeDist < smallestDist) {
                                        indexSmallest = i;
                                        smallestDist = mSPTNeighborStates[i].pTreeDist;
                                    }
                                }
                            }
                        }
                        // save index of father
                        if(mSPTNeighborStates[i].pNeighbor->id == mFatherID) {
                            indexFather = i;
                        }
                    }
                }
                smallestDist++;
                if(indexSmallest != SPT_INVALID_U8) {
                    bool change = false;
                    // change, if father has not smallest TreeID of all neighbors
                    if(smallestTreeID < mSPTNeighborStates[indexFather].pTreeID) {
                        change = true;
                    } else if(smallestTreeID == mSPTNeighborStates[indexFather].pTreeID) {
                        // change if new dist is smaller than fathers distance
                        if(mSPTNeighborStates[indexSmallest].pTreeDist < mSPTNeighborStates[indexFather].pTreeDist) {
                            change = true;
                        }
                    }
                    if(change) {
                        mFatherID = mSPTNeighborStates[indexSmallest].pNeighbor->id;
                        mTreeID = mSPTNeighborStates[indexSmallest].pTreeID;
                    }
                } else if(!mIsRoot){ // if nobody is found with a smaller distance and node is not root
                    smallestDist++;
                }
                didExecute = false;
                // sets execute true if distance has changed
                if(mTreeDist > smallestDist) {
                    didExecute = true;
                    getCout() << (long)NetworkTime::get() << ";SPT_C;v[" << (int)palId_id()
                            << "];f=" << (int) mFatherID << "!\n";
                }
                mTreeDist = smallestDist;
                mMsgAction = SPT_GUARD_MINIMIZE;
            } else {
                mMinimizeCount++;
            }
            break;
        case DIST_ALGO_GUARD_ALONE:
            // if node is alone and not root return to init state
            if(!mIsRoot) {
                mLocalState = SPT_STATE_INIT;
            }
            didExecute = false;
            break;
        default:
            didExecute = false;
            break;
    }
    return didExecute;
}

void SPT2Algo::finally() {
    resetTreeList();
    resetTreeId();
}

void SPT2Algo::setCache(DataIndication& msg) {
    SPT2Header header;
    msg.getAirframe() >> header;
    // if it is a SPT msg
    if(header.pMsgType == SPT_MSG_TYPE_DEFAULT) {
        // search associated entry in nl list
        for(uint8_t i = 0; i < mCacheSize; i++) {
            if(mSPTNeighborStates[i].pNeighbor->id == msg.src) {
                // if it is a discover msg and the node didn't act yet, it activates the node
                if(header.pMsg == SPT_GUARD_DISCOVER && mLocalState == SPT_STATE_INIT) {
                    mLocalState = SPT_STATE_OUT;
                }
                // updates values
                mSPTNeighborStates[i].pState = header.pState;
                mSPTNeighborStates[i].pTreeID = header.pTreeID;
                mSPTNeighborStates[i].pTreeDist = header.pTreeDist;
                // if this node is listed as father, sets src as child
                if(header.pFather == palId_id()) {
                    mSPTNeighborStates[i].pTreeState = SPT_TREE_CHILD;
                } else if(mSPTNeighborStates[i].pTreeState != SPT_TREE_FATHER) {
                    // if src says node is not father, sets state to none, if src is not node's father
                    mSPTNeighborStates[i].pTreeState = SPT_TREE_NONE;
                }
                break;
            }
        }
    }
}

void SPT2Algo::saveLocalState() {
    mBackupState = mLocalState;
    mBackupFather = mFatherID;
    mBackupTreeDist = mTreeDist;
}

void SPT2Algo::revertLocalState() {
    mLocalState = mBackupState;
    mFatherID = mBackupFather;
    mBackupTreeDist = mTreeDist;
}

void SPT2Algo::outReportString() {
    getCout() << "trDD=" << (int) mTreeDist;
    getCout() << ";cache={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                getCout() << "(" << (int)mSPTNeighborStates[i].pNeighbor->id << "," << (int)mSPTNeighborStates[i].pTreeID << "," << (int)mSPTNeighborStates[i].pTreeDist << ")";
                atLeastOne = true;
            }
        }
    }
    getCout() << "}";
}

void SPT2Algo::outAlgoAbbrev() {
    getCout() << "SPT";
}

void SPT2Algo::addHeader(Airframe& frame) {
    uint8_t neighborCount = 0;
    // set header and add it to frame
    SPT2Header header(SPT_MSG_TYPE_DEFAULT,mLocalState,mMsgAction, mFatherID, mTreeID, mTreeDist);
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
            // can only be NEIGHBORLISTSIZE nodes at max
            if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                header.pNeighbors[neighborCount] = mSPTNeighborStates[i].pNeighbor->id;
                neighborCount++;
            }
        }
    }
    frame << header;
}

bool SPT2Algo::checkMsgType(Airframe& frame) {
    // checks whether the msg is of type MIS
    bool result = false;
    uint8_t msgType = 0;
    frame >> msgType;

    if(msgType == SPT_MSG_TYPE_DEFAULT) {
        result = true;
    }
    frame << msgType;
    return result;
}

uint8_t SPT2Algo::getAlgoType() {
    return DIST_ALGO_TYPE_SPT;
}

void SPT2Algo::setRoot(node_t rootID) {
    if(palId_id() == rootID) {
        mIsRoot = true;
        mLocalState = SPT_STATE_IN;
    }
}

void SPT2Algo::resetTreeId() {
    node_t smallestTreeId = palId_id();
    uint16_t smallestTreeDist = SPT_INVALID_U16;
    // only update if node is in tree
    if(mLocalState == SPT_STATE_IN) {
        // iterate through all neighbors
        for(uint8_t i = 0; i < mCacheSize; i++) {
            // if neighbor is there and has bidirectional connection
            if(mSPTNeighborStates[i].pNeighbor->id != SPT_INVALID_ID) {
                if(mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink()) {
                    // if it's not nodes father, check whether it is labeled as such and change if
                    if(mSPTNeighborStates[i].pNeighbor->id != mFatherID) {
                        if(mSPTNeighborStates[i].pTreeState == SPT_TREE_FATHER) {
                            mSPTNeighborStates[i].pTreeState = SPT_TREE_NONE;
                        }
                    } else { // if it is father
                        // check whether it is not labeled as such and change if
                        if(mSPTNeighborStates[i].pTreeState != SPT_TREE_FATHER) {
                            mSPTNeighborStates[i].pTreeState = SPT_TREE_FATHER;
                        }
                        // check fathers tree distance
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

void SPT2Algo::resetTreeList(){
    for(uint8_t i = 0; i < mCacheSize; i++) {
        if(mSPTNeighborStates[i].pState != SPT_STATE_INIT) {
            if((mSPTNeighborStates[i].pNeighbor->id == SPT_INVALID_ID) || (!mSPTNeighborStates[i].pNeighbor->hasBidirectionalLink())) {
                mSPTNeighborStates[i].remove();
            }
        }
    }
}

uint8_t SPT2Algo::getIndexOf(node_t inId){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i < mCacheSize; i++){
        if(mSPTNeighborStates[i].pNeighbor->id == inId) {
            return i;
        }
    }
    return index;
}

SPT2Data* SPT2Algo::getDataPtr() {
    return mSPTNeighborStates;
}

node_t SPT2Algo::getTreeID() {
    return mTreeID;
}

uint16_t SPT2Algo::getTreeDist() {
    return mTreeDist;
}

node_t SPT2Algo::getFatherID() {
    return mFatherID;
}

}
