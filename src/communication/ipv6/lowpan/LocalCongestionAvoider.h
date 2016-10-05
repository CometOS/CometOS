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

#ifndef DELAYPROVIDER_H_
#define DELAYPROVIDER_H_

#include "types.h"
#include "Airframe.h"
#include "LowpanIndication.h"
#include "lowpan-macros.h"
#include "IPv6Datagram.h"
#include "Ieee802154MacAddress.h"
#include "QueueObject.h"

namespace cometos_v6 {

/**
 * Abstracts the mechanism to restrict the sending of frames due the detection
 * of local congestion.
 */
class LocalCongestionAvoider {
public:
    LocalCongestionAvoider() {};

    virtual ~LocalCongestionAvoider() {};

    void manualInit();

    void finish();

    /**
     * return true if congestion avoidance allows sending
     *        false else
     */
    bool sendingAllowed();

    /**
     * @return true, if this node considers itself congested
     *         false else
     */
    bool getCongestionStatus();

    /** The currently processed datagram has changed.
     */
    void switchedQueueObject();

    /** The currently processed datagram has been given up on.
     */
    void abortCurrent();

    /** A frame has been sent to the MAC layer
     *
     * @param[in] fHead
     *      containing fragmentation data for the sent frame, if existing
     *      if not, fHead.isFirstOfDatagram() and fHead.isLastOfDatagram()
     *      return both true
     * @param[in] dst
     *      802.15.4 address of destination of the frame
     */
    void sentFrame(const LowpanFragMetadata& fHead,
                   const Ieee802154MacAddress& dst);

    /**
     * A frame that is not destined to this node has been snooped by
     * the MAC layer.
     *
     * @param[in] dg
     *      datagram containing the IPv6 header; extension headers shall
     *      not be expected to be present. Only contains valid information,
     *      if the snooped frame is the first fragment of a datagram, i.e.,
     *      fHead.isFirstOfDatagram() == true
     * @param[in] src
     *      802.15.4 address of the source of the frame
     * @param[in] fHead
     *      fragmentation data of the snooped frame (only present and valid
     *      if fHead.isFirstOfDatagram() and fHead.isLastOfDatagram() are both
     *      true)
     */
    void snoopedFrame(const IPv6Datagram& dg,
                      const Ieee802154MacAddress& src,
                      const LowpanFragMetadata& fHead);

    /**
     * A frame destined to us has been enqueued.
     *
     * @param[in] dg   datagram belonging to the received fragment
     * @param[in]
     *   dgId ID of the enqueued fragment in terms of 802154 src/dst,
     *   datagram size and the tag we are using
     *
     */
    void enqueueFrame(const IPv6Datagram& dg,
                      const LocalDgId& dgId);

private:

    /** NVI pattern; actual initialize method */
    virtual void doManualInit() = 0;

    /** NVI pattern; actual finish method */
    virtual void doFinish() = 0;

    /** NVI pattern; actual sendingAllowed implementation, to be overridden by subclass */
    virtual bool handleSendingAllowed() = 0;

    /** NVI pattern; actual getCongestionStatus implementation, to be overridden by subclass */
    virtual bool handleGetCongestionStatus() = 0;

    /** NVI pattern; actual switchedQueueObject implementation, to be overridden by subclass */
    virtual void handleSwitchedQueueObject() = 0;

    /** NVI pattern; actual sentFrame implementation, to be overridden by subclass */
    virtual void handleSentFrame(const LowpanFragMetadata& fHead,
                                 const Ieee802154MacAddress& dst) = 0;

    /** NVI pattern; actual snoopedFrame implementation, to be overridden by subclass */
    virtual void handleSnoopedFrame(const IPv6Datagram& dg,
                                    const Ieee802154MacAddress& src,
                                    const LowpanFragMetadata& fHead) = 0;
    /** NVI pattern; actual abortFrame implementation, to be overridden by subclass */
    virtual void handleAbortFrame() = 0;

    /** NVI pattern; actual enqueueFrame implementation, to be overriden by subclass */
    virtual void handleEnqueueFrame(const IPv6Datagram& dg,
                                    const LocalDgId& dgId) = 0;
};

} /* namespace cometos_v6 */
#endif /* DELAYPROVIDER_H_ */
