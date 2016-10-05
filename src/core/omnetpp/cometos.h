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
 * Includes core system of CometOS
 *
 * @author Stefan Unterschütz
 */

#ifndef COMET_OS_H_
#define COMET_OS_H_

/*INCLUDES-------------------------------------------------------------------*/

#define OMNETPP
#define ENABLE_LOGGING
#define COMETOS_NEW

#include <stdint.h>
#include <stdio.h>
#include <omnetpp.h>
#include "Module.h"
#include "Message.h"
#include "List.h"

/*MACROS---------------------------------------------------------------------*/
#define	CONFIG_NED(x)	x = par((#x))
#define CONFIG_NED_OBJ(stat, x) stat.x = par((#x))
#define RECORD_SCALAR(x) recordScalar((#x), x)
#define RECORD_NAMED_SCALAR(name, value) recordScalar(name, value)
#define RECORD_SCALAR_ARRAY(prefix, size, value, valueOffset) \
    {\
        ASSERT(size+valueOffset < 100); \
        std::string _prefixStr(prefix); \
        for (uint8_t i = 0; i < size; i++) {\
            std::stringstream ss; \
            uint8_t k = i + valueOffset; \
            if (k / 10 > 0) {\
                ss << _prefixStr << (char) (k / 10 + 48) << (char) (k % 10 + 48);\
            } else {\
                ss << _prefixStr << (char) (k + 48);\
            }\
            recordScalar(ss.str().c_str(), value[i]);\
        }\
    }

#define STREAM_HEX(x) std::hex << (x) << std::dec
#define ENTER_METHOD_SILENT() Enter_Method_Silent()
#define TAKE_MESSAGE(x) take(x)

#define CANCEL_AND_DELETE(x) cancelAndDelete(x)

#define modifyDisplay(type,index,value) 	if (ev.isGUI()) \
		getParentModule()->getDisplayString().setTagArg(type,index,value)


#define printf_init(x)

namespace cometos {

void initialize();

void run();

void stop();


bool run_once();

}

uint16_t intrand(uint16_t r);

#endif /* COMET_OS_H_ */
