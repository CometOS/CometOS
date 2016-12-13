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

#ifndef CONDENSINGLOWPANQUEUE_H_
#define CONDENSINGLOWPANQUEUE_H_

#include "LowpanQueue.h"
#include "LowpanQueueCommon.h"
#include "QueueObject.h"
#include "QueueFragment.h"
#include "Queue.h"

namespace cometos_v6 {

class InOrderDg;

class NextHopTagStoringQueueObject : public QueueObject {
public:
    NextHopTagStoringQueueObject() :
        nextHopTag(0),
        tagSet(false)
    {}

    virtual bool absorb(QueueObject* other) {
        return false;
    }

    bool isSet() const {
        return tagSet;
    }

    void setTagOfNextHop(uint16_t tag) {
        tagSet = true;
        nextHopTag = tag;
    }

    uint16_t getTagOfNextHop() const {
        return nextHopTag;
    }

private:
    uint16_t nextHopTag;
    bool tagSet;
};


class QueueObjectWrapper : public NextHopTagStoringQueueObject {
public:
    QueueObjectWrapper(QueueObject* obj) :
        theObj(obj)
    {}

    virtual ~QueueObjectWrapper() {
        delete(theObj);
        theObj = NULL;
    }

    virtual QueueObject::response_t response(bool success, const cometos::MacTxInfo & info) {
        return theObj->response(success, info);
    }

    virtual void createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragHead,
                             const IPv6Datagram* & dg) {
        return theObj->createFrame(frame, maxSize, fragHead, dg);
    }

    virtual const Ieee802154MacAddress& getDstMAC() const {
        return theObj->getDstMAC();
    }

    virtual uint8_t getPRCValue() const {
        return theObj->getPRCValue();
    }

    virtual uint8_t getSRCValue() const {
        return theObj->getSRCValue();
    }

    virtual uint8_t getHRCValue() const {
        return theObj->getHRCValue();
    }

    virtual void logEnqueue() const {
        theObj->logEnqueue();
    }

    virtual bool canBeDeleted() const {
        return theObj->canBeDeleted();
    }

    virtual LocalDgId getDgId(bool& valid) const {
        return theObj->getDgId(valid);
    }

    virtual const IPv6Datagram* getDg() const {
        return theObj->getDg();
    }

    virtual uint8_t currOffset() const {
        return theObj->currOffset();
    }

    virtual uint16_t getCurrDgSize() const {
        return theObj->currOffset();
    }

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const {
        return theObj->belongsTo(src, dst, tag, size);
    }

    virtual bool representsDatagram() const {
        return theObj->representsDatagram();
    }

    virtual bool nextFrameReady() {
        return theObj->nextFrameReady();
    }

private:
    QueueObject* theObj;
};

/** Wrapper to QueueObject, which adds singly linked list capabilities */
class InOrderDgFrag {
    friend InOrderDg;
public:
    InOrderDgFrag(QueueObject* qf, InOrderDgFrag* next = NULL);

    virtual ~InOrderDgFrag();

private:
    InOrderDgFrag& operator=(const InOrderDgFrag& rhs);
    InOrderDgFrag(const InOrderDgFrag& rhs);

    QueueObject* qf;
    InOrderDgFrag* next;
};


class InOrderDg : public NextHopTagStoringQueueObject {
public:
    InOrderDg(QueueFragment* firstFrame);

    virtual ~InOrderDg();

    virtual QueueObject::response_t response(bool success, const cometos::MacTxInfo & info);

    virtual void createFrame(cometos::Airframe& frame,
                             uint8_t maxSize,
                             LowpanFragMetadata& fragHead,
                             const IPv6Datagram* & dg);

    virtual const Ieee802154MacAddress& getDstMAC() const;

    virtual uint8_t getPRCValue() const;

    virtual uint8_t getSRCValue() const;

    virtual uint8_t getHRCValue() const;

    virtual void logEnqueue() const;

    virtual bool canBeDeleted() const;

    virtual LocalDgId getDgId(bool& valid) const;

    virtual const IPv6Datagram* getDg() const;

    virtual uint8_t currOffset() const;

    virtual uint16_t getCurrDgSize() const;

    virtual bool absorb(QueueObject* other);

    virtual bool belongsTo(Ieee802154MacAddress const & src,
                           Ieee802154MacAddress const & dst,
                           uint16_t tag,
                           uint16_t size) const;

    virtual bool representsDatagram() const;

    virtual bool nextFrameReady();

private:
    // prevent usage of assignment and copy
    InOrderDg& operator=(const InOrderDg& rhs);
    InOrderDg(const InOrderDg& rhs);

    void removeFront();

    PacketInformation* pi;
    uint8_t newOffset;
    InOrderDgFrag* first;
    LocalDgId dgId;
    bool idValid;
};




class DgOrderedLowpanQueue : public LowpanQueue {
public:
    DgOrderedLowpanQueue(uint8_t queueSize,
                         uint8_t maxFrameSize,
                         const TypedDelegate<cometos::DataResponse> &delegate = TypedDelegate<cometos::DataResponse>()) :
                queue(new cometos::DynSList<NextHopTagStoringQueueObject*>(queueSize)),
                queueSize(queueSize),
                maxFrameSize(maxFrameSize),
                delegate(delegate),
                macControlMode(LOWPAN_MACCONTROL_DEFAULT)
    {}

    ~DgOrderedLowpanQueue() {
        LOG_DEBUG("Deleting objects from queue of size " << (int) queue->size())
        for(uint8_t idx=queue->begin(); idx != queue->end(); idx = queue->next(idx)) {
            QueueObject* qo = (*queue)[idx];
            LOG_DEBUG("Delete " << qo);
            delete qo;
        }
        queue->clear();
        delete queue;
        queue = NULL;
    }

    virtual const QueueObject* getObjectToRespondTo() {
        return lastObjectRetrieved;
    }

    virtual lowpanQueueResponse_t response(cometos::DataResponse * resp) {
        lowpanQueueResponse_t ret = lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS;
        if (lastObjectRetrieved != nullptr) {
            ASSERT(lastObjectRetrieved!=nullptr);
            cometos::MacTxInfo * txInfo = NULL;
            cometos::MacTxInfo dummyInfo;

            if (resp->has<cometos::MacTxInfo>()) {
                txInfo = resp->get<cometos::MacTxInfo>();
            } else {
                txInfo = &dummyInfo;
            }
            QueueObject::response_t res = lastObjectRetrieved->response(resp->isSuccess(), *txInfo);
            LOG_DEBUG("got response " << (int) res << " from inOrderDg");
            switch(res) {
                case QueueObject::QUEUE_DELETE_OBJECT: {
                    removeFromQueue(lastObjectRetrieved);
                    LOG_DEBUG("Delete " << lastObjectRetrieved << " fail");
                    delete lastObjectRetrieved;
                    ret = lowpanQueueResponse_t::LOWPANQUEUE_PACKET_DROPPED;
                    break;
                }
                case QueueObject::QUEUE_PACKET_FINISHED: {
                    removeFromQueue(lastObjectRetrieved);
                    LOG_DEBUG("Delete " << lastObjectRetrieved << " finish");
                    delete lastObjectRetrieved;
                    ret = lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED;
                    break;
                }
                case QueueObject::QUEUE_DELETE_SIMILAR: {
                    ASSERT(false);
                    break;
                }
                case QueueObject::QUEUE_KEEP: {
                    ret = lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS;
                    break;
                }
            }
            lastObjectRetrieved = nullptr;
        } else {
            LOG_WARN("LastObjectRetrieved has been removed from queue in the meantime");
            ret = lowpanQueueResponse_t::LOWPANQUEUE_ERROR;
        }

        delete resp;
        return ret;
    }

    virtual cometos::DataRequest* getNext(LowpanFragMetadata& fragInfo,
                                          const IPv6Datagram* & dg) {
        if (hasNext()) {
            lastObjectRetrieved = queue->get(queue->begin());
            return LowpanQueueCommon::getLowpanQueueFrame(
                    lastObjectRetrieved,
                    maxFrameSize,
                    macControlMode,
                    delegate,
                    fragInfo,
                    dg);
        } else {
            return NULL;
        }
    }

    virtual const QueueObject* getFront() {
        return getActiveObject();
    }

    virtual bool cancelAndDeleteActiveDatagram() {
        if (queue->size() > 0) {
            ASSERT(queue->get(queue->begin()) != NULL);
            QueueObject* qo = queue->get(queue->begin());
            queue->pop_front();
            if (qo == lastObjectRetrieved) {
                lastObjectRetrieved = nullptr;
            }
            delete qo;
            return true;
        } else {
            return false;
        }
    }

    virtual bool add(QueueObject* obj) {
#ifdef ENABLE_LOGGING
        bool idValid;
        LocalDgId id = obj->getDgId(idValid);
        LOG_DEBUG("Enqueue; src=" << id.src.a4() << "|dst=" << id.dst.a4() <<"|tag=" << (int)id.tag);
#endif
        // try to attach new object to existing one that already has
        // information on the represented datagram; if that fails,
        // add it to the end of the queue
        for (uint8_t it = queue->begin(); it != queue->end(); it = queue->next(it)) {
            if ((*queue)[it]->absorb(obj)) {
                LOG_DEBUG("qo@" << obj << " absorbed by inOrderDg@" << (*queue)[it]);
                return true;
            } else {
                // continue iteration to check for next item
            }
        }

        // if we reach this point, queueobject was not absorbed and is either
        // a new fragment or a new datagram from IP layer
        if (queue->full()) {
            return false;
        }
        if (obj->representsDatagram()) {
            queue->push_back(new QueueObjectWrapper(obj));
            LOG_DEBUG("Added as self-contained qo@" << obj << "|queueSize=" << (int) queue->size());
            return true;
        } else {
            // TODO we assume this to be a QueueFragment, because representsDatagram return false
            InOrderDg* newDg = new InOrderDg(static_cast<QueueFragment*>(obj));
            queue->push_back(newDg);
            LOG_DEBUG("Created new InOrderDg@" << newDg << "; first element@" << obj << " queueSize=" << (int) queue->size());
            return true;
        }
    }

    virtual uint8_t removeObjects() {
        uint8_t count = 0;

        for (uint8_t it = queue->begin(); it != queue->end();) {
            NextHopTagStoringQueueObject* qo = (*queue)[it];
            if (qo->canBeDeleted()) {
#ifdef ENABLE_LOGGING
                bool idValid;
                LocalDgId id = qo->getDgId(idValid);
                LOG_DEBUG("Delete queueObject@" << qo
                                           <<"; src=" << cometos::hex << id.src.a4()
                                           << "|dst=" << cometos::hex << id.dst.a4()
                                           << "|tag=" << cometos::dec << id.tag);
#endif
                if (lastObjectRetrieved == qo) {
                    lastObjectRetrieved = nullptr;
                    LOG_DEBUG("Removed pending queue object");
                }
                delete qo;
                it = queue->erase(it);
                count++;
            } else {
                it = queue->next(it);
            }
        }

        return count;
    }

    virtual uint8_t getQueueSize() const {
        return queue->size();
    }

    virtual bool hasNext() const {
        return queue->get(queue->begin())->nextFrameReady();
    }

    virtual uint8_t getQueueMaxSize() const {
        return queue->max_size();
    }

    virtual void setMacControlMode(macControlMode_t mode) {
        if (mode <= LOWPAN_MACCONTROL_MAX_VALUE) {
            macControlMode = mode;
        }
    }

    virtual macControlMode_t getMacControlMode() {
        return macControlMode;
    }



    NextHopTagStoringQueueObject* findTaggedObject(uint16_t tag) {
        for (uint8_t idx = queue->begin();
             idx != queue->end();
             idx = queue->next(idx))
        {
            NextHopTagStoringQueueObject* qo = (*queue)[idx];
            if (qo->getTagOfNextHop() == tag) {
                return qo;
            }
        }
        return NULL;
    }

    NextHopTagStoringQueueObject* getActiveObject() {
        if (queue->begin() != queue->end()) {
            return (*queue)[queue->begin()];
        } else {
            return NULL;
        }
    }

    virtual void pushBack() {
        NextHopTagStoringQueueObject* qo = (*queue)[queue->begin()];
        queue->pop_front();
        queue->push_back(qo);
        LOG_DEBUG("move " << qo << " to end; front=" << (uintptr_t) queue->begin());
    }



private:
    void removeFromQueue(NextHopTagStoringQueueObject* obj) {
        uint8_t lastIndexRetrieved = queue->find(obj);
        ASSERT(lastIndexRetrieved != queue->end());
        queue->erase(lastIndexRetrieved);
    }

    // prevent assignment and copy construction
    DgOrderedLowpanQueue& operator=(const DgOrderedLowpanQueue& rhs);
    DgOrderedLowpanQueue(const DgOrderedLowpanQueue& rhs);

    cometos::SListBase<NextHopTagStoringQueueObject*>* queue;
    uint8_t                              queueSize;
    uint8_t                              maxFrameSize;
    TypedDelegate<cometos::DataResponse> delegate;
    macControlMode_t                     macControlMode;
    NextHopTagStoringQueueObject*        lastObjectRetrieved;
};


} // namespace

#endif /* CONDENSINGLOWPANQUEUE_H_ */
