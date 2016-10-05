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

#ifndef LOWPANQUEUEINTERFACE_H_
#define LOWPANQUEUEINTERFACE_H_

#include "QueueObject.h"
#include "DataResponse.h"

namespace cometos_v6 {

typedef uint8_t macControlMode_t;
enum {
    LOWPAN_MACCONTROL_DEFAULT = 0,
    LOWPAN_MACCONTROL_SRC = 1,
    LOWPAN_MACCONTROL_PRC = 2,
    LOWPAN_MACCONTROL_ASPRC = 3,
    LOWPAN_MACCONTROL_MSPRC = 4,
    LOWPAN_MACCONTROL_MAX_VALUE = LOWPAN_MACCONTROL_MSPRC
};

#if defined SWIG || defined BOARD_python
enum lowpanQueueResponse_t {
#else
enum lowpanQueueResponse_t : uint8_t {
#endif
    LOWPANQUEUE_PACKET_DROPPED,
    LOWPANQUEUE_PACKET_FINISHED,
    LOWPANQUEUE_IN_PROGRESS,
    LOWPANQUEUE_ERROR
}; ///< responses of the method response

class LowpanQueue {
public:
    virtual ~LowpanQueue() {
        ;
    }

    /**
     * Pass response from MAC to the last retrieved QueueObject. Will cause
     * the queue to cleanup any remaining queue objects belonging to the
     * same datagram in case of an error.
     *
     * @param resp response to be passed; ownership passes to the queue
     * @return LOWPANQUEUE_IN_PROGRESS if more frames can be retrieved
     *         LOWPANQUEUE_PACKET_DROPPED if the datagram has been dropped
     *         LOWPANQUEUE_PACKET_FINISHED if the last frame of the datagram
     *                                     has been processed
     *         LOWPANQUEUE_ERROR if some other error occurred
     *
     */
    virtual lowpanQueueResponse_t response(cometos::DataResponse * resp) = 0;

    /**
     * Retrieve the next lowpan frame/fragment to be sent.
     *
     * @param[out] isFirstOfDatagram will be set to true, if the returned frame
     *                               is the first within the datagram
     * @return DataRequest containing an Airframe, which can be sent to the MAC
     */
    virtual cometos::DataRequest* getNext(LowpanFragMetadata& fragInfo,
                                          const IPv6Datagram* & dg) = 0;

    /**
     * Get first element of queue.
     * @return
     *  const pointer to the QueueObject currently at the front of the queue
     */
    virtual const QueueObject* getFront() = 0;

    virtual const QueueObject* getObjectToRespondTo() = 0;

    virtual bool cancelAndDeleteActiveDatagram() = 0;

    /**
     * Add a new queue object to the queue, which represents a whole or part
     * of an IPv6 Datagram.
     */
    virtual bool add(QueueObject* obj) = 0;

    /**
     * Removes all objects from the queue which are marked for deletion.
     * @return number of removed objects
     */
    virtual uint8_t removeObjects() = 0;

    /**
     * @return number of QueueObjects currently enqueued
     */
    virtual uint8_t getQueueSize() const = 0;

    virtual bool hasNext() const = 0;

    /** @return maximum number of QueueObjects this queue can hold. */
    virtual uint8_t getQueueMaxSize() const = 0;

    /** Sets the control mode (number of retransmissions depending on the
     * progress of the datagram.
     * @param[in] mode control mode this queue should use
     */
    virtual void setMacControlMode(macControlMode_t mode) = 0;

    /** @return current control mode used by the queue */
    virtual macControlMode_t getMacControlMode() = 0;

    /** Push back first queue element to the end of the queue */
    virtual void pushBack() = 0;
};


} // namespace cometos_v6

#endif /* LOWPANQUEUEINTERFACE_H_ */
