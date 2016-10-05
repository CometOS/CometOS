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

#include "LocalCongestionAvoider.h"

namespace cometos_v6 {

bool LocalCongestionAvoider::sendingAllowed() {
    bool allowed = handleSendingAllowed();
    return allowed;
}

bool LocalCongestionAvoider::getCongestionStatus() {
    return handleGetCongestionStatus();
}

void LocalCongestionAvoider::switchedQueueObject() {
    handleSwitchedQueueObject();
}

void LocalCongestionAvoider::enqueueFrame(const IPv6Datagram& dg,
                      const LocalDgId& dgId) {
    LOG_DEBUG("Enqueue; src=" << dgId.src.a4() << "|dst=" << dgId.dst.a4() << "|tag=" << dgId.tag <<"|size=" << dgId.size);
    handleEnqueueFrame(dg, dgId);
}

void LocalCongestionAvoider::abortCurrent() {
    LOG_INFO("Aborted current frame");
    handleAbortFrame();
}

void LocalCongestionAvoider::sentFrame(const LowpanFragMetadata& fHead,
                                       const Ieee802154MacAddress& nextHop) {
    LOG_INFO("Sent frame to 0x" << cometos::hex <<  nextHop.a4()
                      << "|dgSize=" << cometos::dec << fHead.dgSize
                      << "|dgSize=" << fHead.dgSize
                      << "|fragSize=" << (int) fHead.size
                      << "|tag=" << (int) fHead.tag
                      << "|offset=" << (int) fHead.offset
                      << "|isFirst=" << (int) fHead.isFirstOfDatagram()
                      << "|isLast=" << (int) fHead.isLastOfDatagram()
                      << "|isFragmented=" << fHead.isFragment());
    handleSentFrame(fHead, nextHop);
}


void LocalCongestionAvoider::snoopedFrame(const IPv6Datagram& dg,
                                          const Ieee802154MacAddress& src,
                                          const LowpanFragMetadata& fHead){
    LOG_INFO("Snooped frame; ipSrc=" << cometos::hex << dg.src.str()
            << "|ipDst=" << dg.dst.str()
            << "|ipSize=" << dg.getUpperLayerPayloadLength() + dg.getCompleteHeaderLength()
            << "|macSrc=" << src.a4()
            << "|dgSize=" << cometos::dec << fHead.dgSize
            << "|fragSize=" << (int) fHead.size
            << "|tag=" << (int) fHead.tag
            << "|offset=" << (int) fHead.offset
            << "|isFirst=" << (int) fHead.isFirstOfDatagram()
            << "|isLast=" << (int) fHead.isLastOfDatagram()
            << "|isFragmented=" << (int) fHead.isFragment());
    handleSnoopedFrame(dg, src, fHead);
}

void LocalCongestionAvoider::manualInit() {
    LOG_INFO("Init LCA");
    doManualInit();
}

void LocalCongestionAvoider::finish() {
    LOG_INFO("Finish LCA");
    doFinish();
}

}
