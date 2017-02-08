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
 * @author Gerry Siegemund, Bastian Weigelt
 */
#include "TZTCAlgo.h"
#include "TZTypes.h"
#include "NetworkTime.h"
#include "palId.h"

//#define OUTPUT_WEIGELT


namespace cometos {

void TZTCAlgo::initialize(node_t ownId){
    mOwnId = ownId;
    pClusterId = mOwnId;
    pClusterDist = 0;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE + STANDBYLISTSIZE;i++){
        neighborView[i].id= TZ_INVALID_ID;
        neighborView[i].lastSeqNum = 0;
        neighborView[i].receivedSinceLastUpdate = 0;
        neighborView[i].ccID = TZ_INVALID_ID;
        neighborView[i].qualityST = 0;
        neighborView[i].qualityLT = 0;
        neighborView[i].onNL = false;
        neighborView[i].tries = 0;
        for(uint8_t j=0; j<NEIGHBORLISTSIZE;j++){
            neighborView[i].neighbors[j] = TZ_INVALID_ID;
        }
    }
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
        pPotentialReplaceIndex[i] = TZ_INVALID_U8;
    }
}

void TZTCAlgo::handle(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn){
    /**
     * asses the quality
     * add if possible
     * update the lists
     * extra timer for list (clearing)!
     */

    //QUALITY
    uint8_t index = getIndexOf(idIn);
    if (index != TZ_INVALID_U8){
        // this assumes that during the TCA interval, only a single packet with TCA header (either beacon or piggybacked) is sent
        neighborView[index].receivedSinceLastUpdate++;
//        // updates quality value for neighbor
//        neighborView[index].updateQuality(headerIn.seqNum, headerIn.ccID, headerIn.ccDist, timeIn);
//        neighborView[index].updateQuality();
        // updates tca entries for neighbor
        neighborView[index].update(headerIn, timeIn);
        if(neighborView[index].qualityLT > Q_MIN){
            checkPromotion(index, headerIn);
        }
        if(neighborView[index].hasBidirectionalLink()) {
            updateClusterId(index);
        }
    } else {
        //ADDING (happens a lot in the beginning, rarely later on)
        index = nextEmptyListSpot();
        if (index != TZ_INVALID_U8){
            neighborView[index].add(idIn, headerIn, timeIn);
        } else {
            // check whether new node has a smaller clusterID then everyone else
            if(headerIn.ccID < pClusterId) {
                checkNewRoot(idIn, headerIn, timeIn);
            }
        }
    }
}

void TZTCAlgo::updateAllQuality() {
    // updates quality values for all on NL and SL
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        if(neighborView[i].id != TZ_INVALID_ID) {
            // update quality
            neighborView[i].updateQuality();
            // if neighbor is on NL and below tolerance minimum it gets kicked
            if(neighborView[i].onNL) {
                if(neighborView[i].qualityLT < Q_TOL) {
                    neighborView[i].remove();
                }
            }
            // checks whether quality is too low to work
            if(neighborView[i].qualityLT < Q_OFF) {

            }
        }
    }
}

bool TZTCAlgo::isBidirOnNL(node_t neighborId) {
    // checks whether there is a Bidirectional link for a neighbor if he is on NL
    bool result = false;
    uint8_t index = getIndexOf(neighborId);
    if(index != TZ_INVALID_U8) {
        if(neighborView[index].hasBidirectionalLink()) {
            result = true;
        }
    }
    return result;
}


node_t TZTCAlgo::checkDeletion(uint8_t inIndex, timestamp_t curTime){
    // checks whether entry with <index> on list can/should be removed and returns its ID if
    node_t removedOfNL = TZ_INVALID_ID;
    if(neighborView[inIndex].id != TZ_INVALID_ID) {
        // If on standbylist and was not promoted after "long" while delete
        if(neighborView[inIndex].timestamp + MAX_EVAL_PERIODE < curTime) {
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_P;v[" << (int)mOwnId << "];-=" << (int) neighborView[inIndex].id << ";i=" << (int) inIndex << "!\n";
#endif
            removedOfNL = neighborView[inIndex].id;
            neighborView[inIndex].remove();
        }
    }

    /**
     * If on neighborlist and drops below Q_TOL it'll be deleted too
     * happends through aging rather than "calculation" initiated by
     * received messages
     */
    if(neighborView[inIndex].onNL == true){
        if(neighborView[inIndex].qualityLT < Q_TOL){
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_Q;v[" << (int)mOwnId << "];-=" << (int) neighborView[inIndex].id << "!\n";
#endif
            removedOfNL = neighborView[inIndex].id;
            neighborView[inIndex].remove();
        }
    } else if(neighborView[inIndex].id != TZ_INVALID_ID) {
        if(neighborView[inIndex].qualityLT < Q_OFF) {
            // if not on NL but below Q_OFF it is a bad link to unstable to work
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_O;v[" << (int)mOwnId << "];-=" << (int) neighborView[inIndex].id << "!\n";
#endif
            removedOfNL = neighborView[inIndex].id;
            neighborView[inIndex].remove();
        }
    }
    return removedOfNL;
}

void TZTCAlgo::checkNewRoot(node_t idIn, TCPWYHeader headerIn, timestamp_t timeIn) {
    uint8_t randStandby = intrand(STANDBYLISTSIZE);
    uint8_t countStandby = 0;
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        if(neighborView[i].id != TZ_INVALID_ID) {
            if(!neighborView[i].onNL) {
                if(neighborView[i].ccID > headerIn.ccID) {
                    if(countStandby >= randStandby) {
                        neighborView[i].remove();
                        neighborView[i].add(idIn, headerIn, timeIn);
                        break;
                    } else {
                        countStandby++;
                    }
                }
            }
        }
    }
}

void TZTCAlgo::checkPromotion(uint8_t inIndex, TCPWYHeader inHeader){
    //Q_MIN ALREADY CHECKED!
    if(!neighborView[inIndex].onNL) {
        if(nodesOnNL() < NEIGHBORLISTSIZE) {
            neighborView[inIndex].join();
        } else {
            checkReplacement(inIndex, inHeader);
        }
    }
}

void TZTCAlgo::checkReplacement(uint8_t inIndex, TCPWYHeader inHeader) {
    resetPotentialReplace();
    uint8_t numReplace = 0;
    numReplace = determineSymmetricReplace(inIndex);
    if(numReplace != 1) {
        numReplace = determineClusterReplace(inIndex, numReplace, inHeader.ccID);
    }
    if(numReplace != 1) {
        numReplace = determineQualityReplace(inIndex, numReplace);
    }
    // increases number of tries if no replacement was found
    if(numReplace != 1) {
        neighborView[inIndex].tries++;
        // if the maximal number of tries is reached removes entry from standby list
        if(neighborView[inIndex].tries >= MAX_TRIES_JOIN) {
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_M;v[" << (int)mOwnId << "];-=" << (int) neighborView[inIndex].id << "!\n";
#endif
            neighborView[inIndex].remove();
        } else if(neighborView[inIndex].ccID == pClusterId && neighborView[inIndex].qualityLT > Q_THRESHOLD) {
            // if neighbor is already in same cluster and has high qualtity he does not need to join anymore
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_K;v[" << (int)mOwnId << "];-=" << (int) neighborView[inIndex].id << "!\n";
#endif
            neighborView[inIndex].remove();
        }
    }
}

uint8_t TZTCAlgo::determineSymmetricReplace(uint8_t inIndex) {
    uint8_t numReplace = 0;
    // checking whether node has bidirectional connection to neighbor
    if(!neighborView[inIndex].hasBidirectionalLink()) {
        for(int i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
            if(i != inIndex) {
                // if that neighbor has no bidirectional link it will be added to potential replacements
                if(neighborView[i].onNL) {
                    if(!(neighborView[i].hasBidirectionalLink())) {
                        numReplace++;
                        addPotentialReplace(i);
                    }
                }
            }
        }
    }
    // if found only one neighbor remove it from list and add new
    if(numReplace == 1) {
#ifdef OUTPUT_WEIGELT
        getCout() << "" << NetworkTime::get() << ";TCA_1;v[" << (int)mOwnId << "];-=" << (int) neighborView[pPotentialReplaceIndex[0]].id << ";+=" << (int) neighborView[inIndex].id << ";i=" << (int)pPotentialReplaceIndex[0] << "!\n";
#endif
        neighborView[pPotentialReplaceIndex[0]].remove();
        neighborView[inIndex].join();
    }
    return numReplace;
}

uint8_t TZTCAlgo::determineClusterReplace(uint8_t inIndex, uint8_t numReplace, node_t newNodeClusterId) {
    // only checks for cluster-based replacement if the new neighbor is from a different cluster
    if(newNodeClusterId != pClusterId) {
        // if there are any neighbors to replace with (they have no bidirectional link)
        if(numReplace > 0) {
            for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
                if(i != inIndex) {
                    if(pPotentialReplaceIndex[i] != TZ_INVALID_U8) {
                        // if asymmetric neighbor has different cluster ID better not to replace him
                        if(neighborView[pPotentialReplaceIndex[i]].ccID != pClusterId) {
                            // if new neighbor and neighbor on list have same cluster ID, try to look whether new neighbor has link to this node
                            if(newNodeClusterId == neighborView[pPotentialReplaceIndex[i]].ccID) {
                                if(neighborView[inIndex].hasLinkToThis()) {
                                    pPotentialReplaceIndex[0] = i;
                                    numReplace = 1;
                                }
                            } else {
                                pPotentialReplaceIndex[i] = TZ_INVALID_U8;
                                numReplace--;
                            }
                        }
                    }
                }
            }
        }
        if(numReplace == 0) { // if no asymmetric neighbor from same cluster is found
            // no need to check cluster IDs of neighbors since by algorithm they share the same when they are bidir.
            // searches for neighbor with shortest distance to cluster head
            uint16_t minDist = pClusterDist;
            node_t highestId = 0;
            for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
                if(i != inIndex) {
                    if(neighborView[i].hasBidirectionalLink()) {
                        if(neighborView[i].ccDist < minDist || neighborView[i].ccID < pClusterId) {
                            if(neighborView[i].id > highestId) {
                                highestId = neighborView[i].id;
                                pPotentialReplaceIndex[0] = i;
                                numReplace++;
                            }
                        }
                    }
                }
            }
            // if there is just one route towards root node discard it
            if(numReplace == 1) {
                numReplace = 0;
            } else if(numReplace > 1) {
                numReplace = 1;
            }
        }
        // if just one replacement left, replace it with new node
        if(numReplace == 1) {
            for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
                if(pPotentialReplaceIndex[i] != TZ_INVALID_U8) {
#ifdef OUTPUT_WEIGELT
                    getCout() << "" << NetworkTime::get() << ";TCA_2;v[" << (int)mOwnId << "];-=" << (int) neighborView[pPotentialReplaceIndex[i]].id << ";+=" << (int) neighborView[inIndex].id << ";i=" << (int)pPotentialReplaceIndex[i] << "!\n";
#endif
                    neighborView[pPotentialReplaceIndex[i]].remove();
                    neighborView[inIndex].join();
                    break;
                }
            }
        }
    }
    return numReplace;
}

uint8_t TZTCAlgo::determineQualityReplace(uint8_t inIndex, uint8_t numReplace) {
    uint16_t lowestQuali = Q_SCALE;
    uint8_t indexLowestQuali = TZ_INVALID_U8;
    if(numReplace > 0) {
        // iterate through all potential replacements and searches for the one with lowest quality
        for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
            if(pPotentialReplaceIndex[i] != TZ_INVALID_U8) {
                if(neighborView[pPotentialReplaceIndex[i]].onNL) {
                    if(neighborView[pPotentialReplaceIndex[i]].qualityLT < lowestQuali) {
                        lowestQuali = neighborView[pPotentialReplaceIndex[i]].qualityLT;
                        indexLowestQuali = pPotentialReplaceIndex[i];
                    }
                }
            }
        }
    } else {
        // if there are no potential replacements (all neighbors have bidir.-link
        //                                          or those without have different clusterId)
        //      searches all neighbors for the one with the lowest quality
        for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
            if(neighborView[i].onNL) {
                if(neighborView[i].qualityLT < lowestQuali) {
                    lowestQuali = neighborView[i].qualityLT;
                    indexLowestQuali = i;
                }
            }
        }
    }
    numReplace = 0;
    // if (<quality of new neighbor> * factor) larger than quality of lowest potential replacement then switch neighbors
    uint32_t qualityValue = neighborView[inIndex].qualityLT * Q_DELTA_FACTOR / Q_SCALE;
    if(qualityValue > lowestQuali) {
#ifdef OUTPUT_WEIGELT
        getCout() << "" << NetworkTime::get() << ";TCA_3;v[" << (int)mOwnId << "];-=" << (int) neighborView[indexLowestQuali].id << ";+=" << (int) neighborView[inIndex].id << ";i=" << (int)indexLowestQuali << "!\n";
#endif
        neighborView[indexLowestQuali].remove();
        neighborView[inIndex].join();
        numReplace = 1;
    } else if(neighborView[inIndex].ccID != pClusterId) { // if new neighbor is not in cluster but all others are pick just the neighbor with lowest q
        if(indexLowestQuali != TZ_INVALID_U8) {
#ifdef OUTPUT_WEIGELT
            getCout() << "" << NetworkTime::get() << ";TCA_4;v[" << (int)mOwnId << "];-=" << (int) neighborView[indexLowestQuali].id << ";+=" << (int) neighborView[inIndex].id << ";i=" << (int)indexLowestQuali << "!\n";
#endif
            neighborView[indexLowestQuali].remove();
            neighborView[inIndex].join();
            numReplace = 1;
        }
    }
    return numReplace;
}

void TZTCAlgo::resetClusterId() {
    //// looks through complete neighbor list to find current cluster ID
    node_t newClusterId = mOwnId;
    uint16_t newClusterDist = 0;
    uint8_t indexOfNew = TZ_INVALID_U8;
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
        // only if the neighbor is on NL and has a Bidirectional link
        if(neighborView[i].hasBidirectionalLink()) {
            if(neighborView[i].ccID < newClusterId) {
                indexOfNew = i;
                newClusterId = neighborView[i].ccID;
                newClusterDist = neighborView[i].ccDist;
            } else if(neighborView[i].ccID == newClusterId) {
                if(neighborView[i].ccDist < newClusterDist) {
                    indexOfNew = i;
                    newClusterDist = neighborView[i].ccDist;
                }
            }
        }
    }
    // if there is one with smaller cluster ID update to his values
    if(indexOfNew != TZ_INVALID_U8) {
        updateClusterId(indexOfNew);
    } else { // else set self as cluster node
        pClusterId = mOwnId;
        pClusterDist = 0;
    }
}

void TZTCAlgo::updateClusterId(uint8_t inIndex) {
    //// updates the local cluster ID from the entry of a neighbor node
    // looks whether cluster ID of neighbor is smaller
    if(neighborView[inIndex].ccID < pClusterId) {
        pClusterId = neighborView[inIndex].ccID;
        // updates distance accordingly
        pClusterDist = neighborView[inIndex].ccDist;
        if(pClusterDist < (uint16_t) - 1) {
            pClusterDist++;
        }
    } else if(neighborView[inIndex].ccID == pClusterId) {
        // if they have the same cluster ID only updates the distance to cluster node
        if(neighborView[inIndex].ccDist < pClusterDist) {
            pClusterDist = neighborView[inIndex].ccDist + 1;
        }
    }
    // if distance gets too large (=> lost contact to cluster root), resets values
    if(pClusterDist > MAX_CLUSTER_DIST){
        pClusterId = palId_id();
        pClusterDist = 0;
    }
}

uint8_t TZTCAlgo::nodesOnNL(){
    uint8_t on = 0;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE+STANDBYLISTSIZE;i++){
        if(neighborView[i].onNL == true) {
            on++;
        }
    }
    return on;
}

void TZTCAlgo::addPotentialReplace(uint8_t index) {
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
        if(pPotentialReplaceIndex[i] == TZ_INVALID_U8) {
            pPotentialReplaceIndex[i] = index;
            break;
        }
    }
}

void TZTCAlgo::resetPotentialReplace() {
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
        pPotentialReplaceIndex[i] = TZ_INVALID_U8;
    }
}

uint8_t TZTCAlgo::getIndexOf(node_t idIn){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE+STANDBYLISTSIZE;i++){
        if(neighborView[i].id == idIn) {
            return i;
        }
    }
    return index;
}

uint8_t TZTCAlgo::nextEmptyListSpot(){
    uint8_t index = TZ_INVALID_U8;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE+STANDBYLISTSIZE;i++){
        if(neighborView[i].id == TZ_INVALID_ID) {
            return i;
        }
    }
    return index;
}

} // namespace cometos
