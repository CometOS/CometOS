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
 * @author Stefan Unterschütz
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "ObjectAggregationExample.h"
#include "NetworkTime.h"
#include "OutputStream.h"

using namespace cometos;

/*METHOD DEFINITION----------------------------------------------------------*/

Define_Module(ObjectAggregationExample);

ObjectAggregationExample::ObjectAggregationExample(const char *name) :
        Module(name), gateIn(this, &ObjectAggregationExample::handleIndication,
                "gateIn"), gateOut(this, "gateOut") {
}

void ObjectAggregationExample::initialize() {
    Module::initialize();
    Message *msg = new Message;
    int value = intrand(100);
    msg->set(new ObjectExample(value));
    getCout() << "Attach meta data with value " << value << cometos::endl;
    schedule(msg, &ObjectAggregationExample::startTransmission, intrand(1000));
}

void ObjectAggregationExample::startTransmission(Message *msg) {
    getCout() << getName() << " timeout at " << NetworkTime::get() << cometos::endl;
    gateOut.send(msg, intrand(2000));
}

void ObjectAggregationExample::handleIndication(Message *msg) {
    ObjectExample* obj = msg->get<ObjectExample>();
    ASSERT(obj!=NULL);
    getCout() << getName() << " receives message at " << NetworkTime::get()
            << " with value " << obj->value << cometos::endl;
    delete msg;
}
