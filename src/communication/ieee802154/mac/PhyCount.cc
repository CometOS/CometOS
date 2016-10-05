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

#include <PhyCount.h>

namespace cometos {

void serialize(ByteVector& buf, const PhyCounts& val) {
    serialize(buf, val.numTo);
    serialize(buf, val.numRxStart);
    serialize(buf, val.numRxSuccess);
    serialize(buf, val.numCrcFail);
    serialize(buf, val.numStartDl);
    serialize(buf, val.numNoBufTosAdapt);
    serialize(buf, val.numNoBufMbl);
    serialize(buf, val.numRxEnd);
}

void unserialize(ByteVector& buf, PhyCounts& val) {
    unserialize(buf, val.numRxEnd);
    unserialize(buf, val.numNoBufMbl);
    unserialize(buf, val.numNoBufTosAdapt);
    unserialize(buf, val.numStartDl);
    unserialize(buf, val.numCrcFail);
    unserialize(buf, val.numRxSuccess);
    unserialize(buf, val.numRxStart);
    unserialize(buf, val.numTo);
}

}
