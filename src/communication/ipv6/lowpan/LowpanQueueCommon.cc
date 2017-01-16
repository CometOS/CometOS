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

#include "LowpanQueueCommon.h"
#include "lowpan-macros.h"
#include "lowpanconfig.h"

namespace cometos_v6 {

cometos::DataRequest* LowpanQueueCommon::getLowpanQueueFrame(
        QueueObject* qo,
        uint8_t maxSize,
        macControlMode_t macControl,
        const TypedDelegate<cometos::DataResponse> &delegate,
        LowpanFragMetadata& fragMeta,
        const IPv6Datagram* & dg)
{
    cometos::AirframePtr frame = cometos::make_checked<cometos::Airframe>();

    qo->createFrame(*frame, maxSize, fragMeta, dg);
    if(frame->getLength() == 0) return NULL; // for LFFRPacket

    cometos::DataRequest* mrq = new cometos::DataRequest(
            qo->getDstMAC().a4(),
            frame,
            delegate);

    if (macControl == LOWPAN_MACCONTROL_DEFAULT) {
        // just pass the packet to the MAC without setting any special
        // parameters
        return mrq;
    } else {
        // we use the retry control of 6lowpan to overwrite the actual
        // mac settings with regard to retransmissions
        uint8_t retries = LOWPAN_RC_MIN_RETRIES;
        switch (macControl) {
        case LOWPAN_MACCONTROL_SRC:
            retries += (LOWPAN_RC_FACTOR * qo->getSRCValue()) / 255;
            break;
        case LOWPAN_MACCONTROL_PRC:
            retries += (LOWPAN_RC_FACTOR * qo->getPRCValue()) / 255;
            break;
        case LOWPAN_MACCONTROL_ASPRC:
            retries += ((LOWPAN_RC_AS_FACTOR * qo->getSRCValue()) / 255) +
                            ((LOWPAN_RC_AP_FACTOR * qo->getPRCValue()) / 255);
            break;
        case LOWPAN_MACCONTROL_MSPRC:
            retries += ((uint32_t) LOWPAN_RC_FACTOR * qo->getSRCValue() * qo->getPRCValue()) /
                            ((uint16_t) 255 * 255);
            break;
        }
        if (retries > LOWPAN_RC_MAX_RETRIES) retries = LOWPAN_RC_MAX_RETRIES;

        mrq->set<cometos::MacMetaRetries>(new cometos::MacMetaRetries(retries));

        return mrq;
    }

}

} // namespace
