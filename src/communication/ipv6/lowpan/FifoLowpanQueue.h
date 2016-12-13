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
 * @author Martin Ringwelski, Andreas Weigel
 */
#ifndef LOWPANQUEUE_H_
#define LOWPANQUEUE_H_

#include "cometos.h"
#include "QueueObject.h"
#include "MacMetaRetries.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "lowpanconfig.h"
#include "LowpanQueue.h"
#include "LowpanQueueCommon.h"
#include "MacAbstractionBase.h"

namespace cometos_v6 {

class FifoLowpanQueue : public LowpanQueue {
public:
    FifoLowpanQueue(uint8_t queueSize,
                    uint8_t maxFrameSize,
                    const TypedDelegate<cometos::DataResponse> &delegate):
        lastObjectRetrievedFrom(nullptr),
        sendingQueue(new QueueObject*[queueSize]),
        queueSize(queueSize),
        qHead(0),
        qSize(0),
        maxFrameSize(maxFrameSize),
        delegate(delegate),
        macControlMode(LOWPAN_MACCONTROL_DEFAULT)
    {}


    ~FifoLowpanQueue() {
        for (uint8_t i = 0; i < qSize; i++) {
            uint16_t pos = qHead + i;
            if (pos >= queueSize) pos -= queueSize;
            delete sendingQueue[pos];
            sendingQueue[pos] = NULL;
        }
        delete[] sendingQueue;
        sendingQueue = NULL;
    }

    virtual const QueueObject* getObjectToRespondTo() {
        return lastObjectRetrievedFrom;
    }

    virtual lowpanQueueResponse_t response(cometos::DataResponse * resp) {

        QueueObject* qo = getQueueHead();
        lowpanQueueResponse_t ret = LOWPANQUEUE_IN_PROGRESS;

        if ((qo != NULL)) {
            if (qo == lastObjectRetrievedFrom) {
                lastObjectRetrievedFrom = nullptr;
                cometos::MacTxInfo * txInfo = NULL;
                cometos::MacTxInfo dummyInfo;

                if (resp->has<cometos::MacTxInfo>()) {
                    txInfo = resp->get<cometos::MacTxInfo>();
                } else {
                    txInfo = &dummyInfo;
                }

                QueueObject::response_t qor = qo->response(resp->isSuccess(), *txInfo);
                if (qor == QueueObject::QUEUE_DELETE_OBJECT || qor == QueueObject::QUEUE_PACKET_FINISHED) {
                    removeQueueHead();
                } else if (qor == QueueObject::QUEUE_DELETE_SIMILAR) {
                    removeQueueHead();
                    removeObjects();
                }

                if (resp->isSuccess() && qor == QueueObject::QUEUE_PACKET_FINISHED) {
                    ret = LOWPANQUEUE_PACKET_FINISHED;
                } else if (!(resp->isSuccess())) {
                    ret = LOWPANQUEUE_PACKET_DROPPED;
                }
            } else {
                // this queue implementation assumes that no one messes with its internal
                // structure; pushBack is not implemented, therefore, if the current
                // front qo is not equal the last retrieved one, we do nothing as we assume
                // that the current object has been remove due to timeout or error condition
                LOG_DEBUG("Response to different queue object");
            }
        } else {
            ret = LOWPANQUEUE_ERROR;
        }
        delete resp;

        return ret;
    }

    virtual cometos::DataRequest* getNext(LowpanFragMetadata& fragInfo,
                                          const IPv6Datagram* & dg) {

        if (qSize > 0) {
            lastObjectRetrievedFrom = getQueueHead();
            LOG_DEBUG("Retrieved qo@" << cometos::hex << (uintptr_t) lastObjectRetrievedFrom);
            return LowpanQueueCommon::getLowpanQueueFrame(
                    lastObjectRetrievedFrom,
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
        return getQueueHead();
    }

    virtual bool cancelAndDeleteActiveDatagram() {
        if (getQueueSize() > 0 && sendingQueue[qHead] != NULL) {
            // simulate a 'no success' response to make sure all frames belonging to
            // the same datagram are deleted as well
            sendingQueue[qHead]->response(false, cometos::MacTxInfo());
            // then delete the front and every frame belonging to it
            removeQueueHead();
            removeObjects();
            return true;
        } else {
            return false;
        }
    }

    virtual bool add(QueueObject* obj) {
        return addQueueObject(obj);
    }

    virtual uint8_t removeObjects() {
        uint8_t pos = qHead;
        uint8_t wpos = qHead;
        uint8_t del = 0;
        for (uint8_t i = 0; i < qSize; i++) {
            if (sendingQueue[pos]->canBeDeleted()) {
                if (lastObjectRetrievedFrom == sendingQueue[pos]) {
                    LOG_DEBUG("Removed pending QueueObject " << lastObjectRetrievedFrom);
                    lastObjectRetrievedFrom = nullptr;
                }
                delete sendingQueue[pos];
                sendingQueue[pos] = NULL;
                del++;
            } else {
                if (wpos != pos) {
                    sendingQueue[wpos] = sendingQueue[pos];
                }
                wpos++;
                if (wpos == queueSize) wpos = 0;
            }
            pos++;
            if (pos == queueSize) pos = 0;
        }
        qSize -= del;
        return del;
    }

    virtual inline uint8_t getQueueSize() const {
        return qSize;
    }

    virtual bool hasNext() const {
        return qSize > 0;
    }

    virtual inline uint8_t getQueueMaxSize() const {
        return queueSize;
    }

    virtual inline void setMacControlMode(macControlMode_t mode) {
        if (mode <= LOWPAN_MACCONTROL_MAX_VALUE) {
            macControlMode = mode;
        }
    }

    virtual inline macControlMode_t getMacControlMode() {
        return macControlMode;
    }

    virtual void pushBack() {
        LOG_ERROR("not implemented");
        ASSERT(false);
    }


private:
    /**
     * Remove the first entry in the queue.
     */
    void removeQueueHead() {
        if (qSize > 0) {
            if (sendingQueue[qHead] != NULL) {
                delete sendingQueue[qHead];
            }
            sendingQueue[qHead] = NULL;
            qHead++;
            if (qHead == queueSize) qHead = 0;
            qSize--;
        }
    }

    /**
     * returns the first QueueObject in the queue
     */
    QueueObject* getQueueHead() {
        if (qSize > 0) {
            return sendingQueue[qHead];
        }
        return NULL;
    }

    /**
     * Adds an object to the end of the queue. Returns false if the queue is
     * full.
     */
    bool addQueueObject(QueueObject* obj) {
        if (qSize < queueSize) {
            uint16_t pos = qHead + qSize;
            if (pos >= queueSize) pos -= queueSize;
            sendingQueue[pos] = obj;
            qSize++;
            return true;
        }
        return false;
    }

    QueueObject*                        lastObjectRetrievedFrom;
    QueueObject**                       sendingQueue;
    uint8_t                             queueSize;
    uint8_t                             qHead;
    uint8_t                             qSize;
    uint8_t                             maxFrameSize;
    TypedDelegate<cometos::DataResponse>   delegate;

    macControlMode_t    macControlMode;
};



//template <uint8_t QueueSize>
//class FifoLowpanQueue : public FifoLowpanQueue {
//public:
//    FifoLowpanQueue(uint8_t maxFrameSize,
//                    const TypedDelegate<cometos::DataResponse> &delegate = TypedDelegate<cometos::DataResponse>()):
//                FifoLowpanQueue(maxFrameSize, QueueSize, &sendingQueue, delegate)
//    {}
//private:
//    QueueObject* sendingQueue[QueueSize];
//};

} /* namespace cometos_v6 */
#endif /* LOWPANQUEUE_H_ */
