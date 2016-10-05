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

#include "MacAbstractionBase.h"

namespace cometos {


// we do not want to have a valid serialization for
// TimeSyncInfo because it is an inherently bad idea to send
// local timestamps over a network; therefore, serialize is not defined here
void unserialize(ByteVector& buf, TimeSyncInfo& val) {
    ASSERT(false);
}


void serialize(ByteVector& buf, const MacTxInfo& val) {
    serialize(buf, val.ackRssi);
    serialize(buf, val.destination);
    serialize(buf, val.numCCARetries);
    serialize(buf, val.numRetries);
    serialize(buf, val.remoteRssi);
    serialize(buf, val.txDuration);
}

void unserialize(ByteVector& buf, MacTxInfo& val) {
    unserialize(buf, val.txDuration);
    unserialize(buf, val.remoteRssi);
    unserialize(buf, val.numRetries);
    unserialize(buf, val.numCCARetries);
    unserialize(buf, val.destination);
    unserialize(buf, val.ackRssi);
}

void serialize(ByteVector& buf, const MacRxInfo& val) {
    serialize(buf, val.lqi);
    serialize(buf, val.lqiIsValid);
    serialize(buf, val.rssi);
}

void unserialize(ByteVector& buf, MacRxInfo& val) {
    unserialize(buf, val.rssi);
    unserialize(buf, val.lqiIsValid);
    unserialize(buf, val.lqi);
}


}
