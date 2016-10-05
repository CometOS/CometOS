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

#include "MacStats.h"

namespace cometos {

void serialize(ByteVector& buf, const MacStats& val) {
    serialize(buf, val.numCSMAFails);
	serialize(buf, val.numCCARetries);
	serialize(buf, val.numIncomingPacketsDropped);
	serialize(buf, val.numOutgoingPackets);
	serialize(buf, val.numOutgoingPacketsAcked);
	serialize(buf, val.numOutgoingPacketsNotAcked);
	serialize(buf, val.numPacketRetries);
	serialize(buf, val.numPacketsDroppedQueue);
	serialize(buf, val.retryCounter);
}

void unserialize(ByteVector& buf, MacStats& val) {
    unserialize(buf, val.retryCounter);
    unserialize(buf, val.numPacketsDroppedQueue);
	unserialize(buf, val.numPacketRetries);
	unserialize(buf, val.numOutgoingPacketsNotAcked);
	unserialize(buf, val.numOutgoingPacketsAcked);
	unserialize(buf, val.numOutgoingPackets);
	unserialize(buf, val.numIncomingPacketsDropped);
	unserialize(buf, val.numCCARetries);
	unserialize(buf, val.numCSMAFails);
}


} // namespace cometos
