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

#include "DSMEMessageBuffer.h"

using namespace cometos;

namespace dsme {

DSMEMessageBuffer::DSMEMessageBuffer() :
        front(buffer), back(buffer + BUFFERSIZE - 1) {

    for (uint16_t i = 0; i < BUFFERSIZE; i++) {
        AirframePtr frame = make_checked<Airframe>();
        airframeStack.push(frame);
    }

    for (uint16_t i = 0; i < BUFFERSIZE - 1; i++) {
        buffer[i].next = &(buffer[i + 1]);
    }
    buffer[BUFFERSIZE - 1].next = nullptr;
}

DSMEMessageBuffer::~DSMEMessageBuffer() {
    while(airframeStack.getSize() != 0) {
        AirframePtr p = airframeStack.top();
        airframeStack.top().reset();
        airframeStack.pop();
        p.deleteObject();
    }
}

DSMEMessage* DSMEMessageBuffer::aquire() {
    DSMEMessageBufferElement *element = nullptr;

    if (front == nullptr) {
        ASSERT(false);
        return nullptr;
    }

    palExec_atomicBegin();
    {
        element = front;
        front = front->next;

        element->message.prepare(airframeStack.top());
        airframeStack.top().reset();
        airframeStack.pop();
    }
    palExec_atomicEnd();

    return &(element->message);
}

DSMEMessage* DSMEMessageBuffer::aquire(cometos::AirframePtr frame) {
    DSMEMessageBufferElement *element = nullptr;

    if (front == nullptr) {
        ASSERT(false);
        return nullptr;
    }

    palExec_atomicBegin();
    {
        element = front;
        front = front->next;

        element->message.prepare(frame);
    }
    palExec_atomicEnd();

    return &(element->message);
}

void DSMEMessageBuffer::release(DSMEMessage* message) {
    if (message == nullptr) {
        return;
    }

    ASSERT(!message->frame || message->frame.unique());
    AirframePtr frame = message->decapsulateFrame();

    palExec_atomicBegin();
    {
        if(!airframeStack.isFull()) {
            if(!frame) {
                frame = make_checked<Airframe>(); // = make_checked<Airframe>(); // the Airframe was decapsulated before
            }
            else {
                frame->clear();
                frame->removeAll();
            }
            bool result = airframeStack.push(frame);
            ASSERT(result);
        }
        else if(frame) {
            frame.deleteObject(); // the stack is full anyway
        }
    }
    palExec_atomicEnd();

    DSMEMessageBufferElement *element = (DSMEMessageBufferElement*) message;

    palExec_atomicBegin();
    {
        back->next = element;
        back = element;

        if (front == nullptr) {
            front = element;
        }
    }
    palExec_atomicEnd();
    return;
}
}
