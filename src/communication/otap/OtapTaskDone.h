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

#ifndef OTAPTASKDONE_H_
#define OTAPTASKDONE_H_

#include "palFirmware.h"
#include "firmwareVersion.h"
#include "Vector.h"

namespace cometos {

enum {
    OTAP_IDLE,
    OTAP_ERASE,
    OTAP_VERIFY,
    OTAP_WRITE
};
typedef uint8_t otap_state_t;

struct OtapTaskDone {
    OtapTaskDone(uint8_t status = PAL_FIRMWARE_SUCCESS,
                 uint8_t opId=OTAP_IDLE,
                 palFirmware_segNum_t size = 0,
                 firmwareVersion_t version = 0) :
        status(status),
        opId(opId),
        size(size),
        version(version)
    {}

    enum {
        STATUS_MASK=0x1F,
        ID_MASK=0xE0,
        ID_START=5
    };

    uint8_t status:ID_START;
    uint8_t opId:(8-ID_START);
    palFirmware_segNum_t size;
    firmwareVersion_t version;
};


void serialize(ByteVector & buf, const OtapTaskDone & val);
void unserialize(ByteVector & buf, OtapTaskDone & val);

}

#endif /* OTAPTASKDONE_H_ */
