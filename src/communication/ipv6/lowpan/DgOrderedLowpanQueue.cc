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
 * @author Andreas Weigel
 */

#include "DgOrderedLowpanQueue.h"

namespace cometos_v6 {

InOrderDgFrag::InOrderDgFrag(QueueObject* qf, InOrderDgFrag* next) :
       qf(qf),
       next(next)
{}

InOrderDgFrag::~InOrderDgFrag() {
    delete(qf);
    qf=NULL;
    next = NULL;
}

InOrderDg::InOrderDg(QueueFragment* firstFrame) :
        dgId(firstFrame->getDgId(idValid).src,
             firstFrame->getDgId(idValid).dst,
             firstFrame->getDgId(idValid).tag,
             firstFrame->getDgId(idValid).size)
{
    ASSERT(idValid);
    ASSERT(firstFrame != NULL);
    pi = firstFrame->getDirectPacket();
    ASSERT(pi != NULL);
    first = new InOrderDgFrag(firstFrame);
}

InOrderDg::~InOrderDg() {
    if (pi != NULL && (!pi->isFree())) {
        pi->setFree(false);
    }
    while (first!=NULL) {
        removeFront();
    }
}

QueueObject::response_t InOrderDg::response(bool success, const cometos::MacTxInfo & info) {
    ASSERT(first!=NULL);
    ASSERT(first->qf!=NULL);
    response_t response = first->qf->response(success, info);
    LOG_DEBUG("got response " << (int) response << " from qf");
    switch(response){
        case QUEUE_DELETE_OBJECT: {
            // success, just remove object
            removeFront();
            return QUEUE_KEEP;
        }
        case QUEUE_DELETE_SIMILAR: {
            // failure, signal to remove whole datagram
            return QUEUE_DELETE_OBJECT;
        }
        case QUEUE_PACKET_FINISHED: {
            // success, whole datagram finished
            return QUEUE_PACKET_FINISHED;
        }
        default:
            ASSERT(false);
            return QUEUE_DELETE_OBJECT;
    }
}

void InOrderDg::removeFront() {
    ASSERT(first!=NULL);
    InOrderDgFrag* tmp = first->next;
    LOG_DEBUG("Delete " << first << " with qf " << first->qf);
    delete first;
    first = tmp;
}

void InOrderDg::createFrame(cometos::Airframe& frame,
                            uint8_t maxSize,
                            LowpanFragMetadata& fragHead,
                            const IPv6Datagram* & dg) {
    ASSERT(first!=NULL);
    newOffset = first->qf->currOffset();
    first->qf->createFrame(frame, maxSize, fragHead, dg);
    newOffset += fragHead.size >> LOWPAN_FRAG_OFFSET_SHIFT;
    LOG_DEBUG("next fragment's offset: " << (uint16_t) newOffset << "|curr=" << (int) fragHead.offset << "|dgSize=" << (int) fragHead.size);
}

const Ieee802154MacAddress& InOrderDg::getDstMAC() const {
    return dgId.dst;
}

uint8_t InOrderDg::getPRCValue() const {
    ASSERT(first!=NULL);
    return first->qf->getPRCValue();
}

uint8_t InOrderDg::getSRCValue() const {
    ASSERT(first!=NULL);
    return first->qf->getSRCValue();
}

uint8_t InOrderDg::getHRCValue() const {
    ASSERT(first!=NULL);
    return first->qf->getHRCValue();
}

void InOrderDg::logEnqueue() const {
    LOG_DEBUG("InOrderDg added to Queue");
}

bool InOrderDg::canBeDeleted() const {
    return pi->isFree();
}

LocalDgId InOrderDg::getDgId(bool& valid) const{
    valid = true;
    return dgId;
}

const IPv6Datagram* InOrderDg::getDg() const {
    return pi->getDatagram();
}

bool InOrderDg::absorb(QueueObject* other) {
    if (other->belongsTo(dgId.src,
            dgId.dst,
            dgId.tag,
            dgId.size))
    {
        if (first == NULL) {
            first = new InOrderDgFrag(other);
            LOG_DEBUG("Added new frag " << first->qf << " at first position");
        } else {
            InOrderDgFrag* tmp = first;
            while (tmp->next != NULL) {
                tmp=tmp->next;
            }
            tmp->next = new InOrderDgFrag(other);
            LOG_DEBUG("Added new frag " << tmp->next->qf << " after " << tmp->qf);
        }
        return true;
    } else {
        return false;
    }
}

bool InOrderDg::belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const {
    return dgId.src == src
            && dgId.dst == dst
            && dgId.tag == tag
            && dgId.size == size;
}

uint8_t InOrderDg::currOffset() const {
    if (first != NULL) {
        return first->qf->currOffset();
    } else {
        // if no next fragment is available, return calculated supposed offset
        return newOffset;
    }
}

uint16_t InOrderDg::getCurrDgSize() const {
    return pi->getSize();
}

bool InOrderDg::representsDatagram() const {
    return true;
}

bool InOrderDg::nextFrameReady() {
    return first != NULL;
}



} // namespace
