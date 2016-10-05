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

#ifndef OTAPASYNC_H_
#define OTAPASYNC_H_

#include "OtapBase.h"
#include "OtapTaskDone.h"

namespace cometos {

class OtapAsync: public OtapBase {
public:
    OtapAsync();
    virtual ~OtapAsync();

    virtual void recvSegment(uint8_t * data, uint16_t segId);

    void initialize();

    /**Prepare a slot for a new firmware.
     *
     * @param slot  slot for new firmware
     * @param size  size of new firmware (number of segments)
     */
    uint8_t initiate(OtapInitMessage & msg);

    /**
     * Verify image in given slot
     */
    uint8_t verify(palFirmware_slotNum_t & slot);


    static void eraseDone(palFirmware_ret_t result);
    static void writeDone(palFirmware_ret_t result);
    static void verifyDone(palFirmware_ret_t result);

    static OtapAsync* getCurrInstance();

private:
    static OtapAsync* instance;

    palFirmware_slotNum_t tmpSlot;
    palFirmware_segNum_t  tmpSegNum;
    uint32_t addressOffset;

    otap_state_t state;

    static const char* EVENT_DONE_NAME;

    uint16_t targetCrc;

    Message crcUpdateMsg;

    OtapTaskDone eventData;
    RemoteEvent<OtapTaskDone> eventDone;
};

} /* namespace cometos */
#endif /* OTAPASYNC_H_ */
