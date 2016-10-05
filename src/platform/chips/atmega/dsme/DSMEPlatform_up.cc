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

#include "DSMEPlatform.h"
#include "DSME.h"

using namespace cometos;

namespace dsme {

void DSMEPlatform::handleDataMessageFromMCPS(uint8_t* msg, uint8_t length, IEEE802154MacAddress src, IEEE802154MacAddress dst) {
    cometos::Airframe *frame = AirframeBuffer::getInstance().aquire();

    frame->setLength(length);
    memcpy(frame->getData(), msg, length);
    cometos::DataIndication* ind = new cometos::DataIndication(frame, src.getShortAddress(), dst.getShortAddress());
    this->gateIndOut.send(ind);

    delete ind;
    AirframeBuffer::getInstance().release(frame);
}

/* handles sending a message from upper layers via DSME */
void DSMEPlatform::handleRequest(DataRequest* msg) {
    IEEE802154MacAddress dstAddr(msg->dst);
    this->dsmeAdaptionLayer.sendMessage(msg->getAirframe().getData(), msg->getAirframe().getLength(), dstAddr);

    // TODO correct response
    msg->response(new DataResponse(true));
    return;
}

class HandleMessageTask : public cometos::Task {
public:
    HandleMessageTask() :
            message(nullptr) {
    }

    void set(DSMEMessage* message, DSMEPlatform::receive_delegate_t receiveDelegate) {
        palExec_atomicBegin();
        {
            this->message = message;
            this->receiveDelegate = receiveDelegate;
        }
        palExec_atomicEnd();
        return;
    }

    virtual void invoke() {
        DSMEMessage* copy = nullptr;
        palExec_atomicBegin();
        {
            copy = message;
        }
        palExec_atomicEnd();

        receiveDelegate(copy);

        palExec_atomicBegin();
        {
            message = nullptr;
        }
        palExec_atomicEnd();
    }
private:
    DSMEMessage* message;
    DSMEPlatform::receive_delegate_t receiveDelegate;
};

void DSMEPlatform::handleReceivedMessageFromAckLayer(DSMEMessage* message) {
    static HandleMessageTask task;
    task.set(message, this->receiveFromAckLayerDelegate);
    cometos::getScheduler().replace(task);
}

void DSMEPlatform::setReceiveDelegate(receive_delegate_t receiveDelegate) {
    this->receiveFromAckLayerDelegate = receiveDelegate;
}

}
