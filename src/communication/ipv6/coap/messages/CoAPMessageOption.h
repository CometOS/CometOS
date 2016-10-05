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
 * @author: Marvin Burmeister, Martin Ringwelski
 */

#ifndef COAPPACKETOPTION_H_
#define COAPPACKETOPTION_H_

#include "cometos.h"
#include "UDPLayer.h"
#include "LongScheduleModule.h"
#include "LowpanAdaptionLayer.h"
#include "coapconfig.h"

namespace cometos_v6 {

class CoAPMessageOption {
public:
    enum optionNumber_t {
        OPT_IFMATCH =        1,
        OPT_URIHOST =        3,
        OPT_ETAG =           4,
        OPT_IFNONMATCH =     5,
        OPT_URIPORT =        7,
        OPT_LOCATIONPATH =   8,
        OPT_URIPATH =        11,
        OPT_CONTENTFORMAT =  12,
        OPT_MAXAGE =         14,
        OPT_URIQUERY =       15,
        OPT_ACCEPT =         17,
        OPT_LOCATIONQUERY =  20,
        OPT_PROXYURI =       35,
        OPT_PROXYSCHEME =    39,
        OPT_SIZE1 =          60
    };

    CoAPMessageOption(CoAPBuffer_t* buffer, uint16_t optNr,
            const char* optData, uint16_t optLen, uint8_t optOrder);

    virtual ~CoAPMessageOption();

    bool optionsContain(uint16_t opNr);

    uint16_t getOptionNr() const{
        return (bufferInfo->getContent()[1]<<8) | bufferInfo->getContent()[0];
    }
    uint8_t getOptionOrder() const{
        return bufferInfo->getContent()[2];
    }
    uint16_t getOptionLen() const{
            return bufferInfo->getSize() - 3;
    }
    const uint8_t* getOptionData() const{
        return &bufferInfo->getContent()[3];
    }

    const uint8_t* getData() const{
        return bufferInfo->getContent();
    }
    uint16_t getDataLen() const{
        return bufferInfo->getSize();
    }

    bool isValid() const {
        return (bufferInfo != NULL);
    }

private:
    BufferInformation* bufferInfo;
};

}
#endif /* COAPPACKETOPTION_H_ */
