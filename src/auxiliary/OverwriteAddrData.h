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

#ifndef ORIGINALADDRDATA_H_
#define ORIGINALADDRDATA_H_

#include "Object.h"

/**
 * Metaobject used to OVERWRITE address information.
 * All single-hop communication layers should check for an object of
 * this type and use the contained information to mark the message to be
 * sent as if it originated from the node given in this object.
 *
 * Mainly used for Serial-to-TCP or Serial-to-Wireless bridges, which
 * do not have an meaningful ID on their own (or do not even implement
 * PAL_ID). The bridge module then can attach such an object to the
 * DataRequest and remain transparent for the receiver of the message,
 * because the lower layer impersonates the original sender.
 */
struct OverwriteAddrData: public cometos::Object {
    virtual ~OverwriteAddrData() {
    }
    OverwriteAddrData(node_t src, node_t dst) :
            src(src), dst(dst) {
    }

    cometos::Object* getCopy() const {
        return new OverwriteAddrData(this->src, this->dst);
    }

    node_t src;
    node_t dst;
};



#endif /* ORIGINALADDRDATA_H_ */
