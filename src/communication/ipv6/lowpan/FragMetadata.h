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

#ifndef LOWPAN_FRAG_METADATA_H_
#define LOWPAN_FRAG_METADATA_H_

namespace cometos_v6 {

#include "cometos.h"

struct LowpanFragMetadata {
    LowpanFragMetadata(uint8_t  size = 0,
                       uint8_t  offset = 0,
                       uint16_t dgSize = 0,
                       uint16_t tag = 0,
                       uint8_t lffrSeq=0):
        isFirst(true),
        lffrSeq(lffrSeq),
        size(size),
        offset(offset),
        dgSize(dgSize),
        tag(tag)
    {}

    bool isFirstOfDatagram() const {
        return offset == 0 && isFirst;
    }

    bool isLastOfDatagram() const {
        return (offsetToByteSize(offset) + size) == dgSize;
    }

    bool isFragment() const {
        return fragmented == 1;
    }

    bool isCongestionFlagSet() const {
        return congestionFlag == 1;
    }

    uint8_t  isFirst:1;
    uint8_t  lffrSeq:5;
    uint8_t  congestionFlag:1;
    uint8_t  fragmented:1;
    uint8_t  size;
    uint8_t  offset;
    uint16_t dgSize;
    uint16_t tag;
};

}

#endif
