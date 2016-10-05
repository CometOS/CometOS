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

#ifndef COMETOS_H_
#define COMETOS_H_

#include <stdio.h>

#include "TaskScheduler.h"

#include "Message.h"
#include "Object.h"
#include "Module.h"
#include "Callback.h"
#include "cometosAssert.h"

#define	CONFIG_NED(x)
#define CONFIG_NED_OBJ(obj, x)
#define RECORD_SCALAR(x)
#define RECORD_NAMED_SCALAR(name, value)
#define RECORD_SCALAR_ARRAY(prefix, size, value, valueOffset)
#if defined BOARD_python or defined BOARD_local
#define STREAM_HEX(x) std::hex << (x) << std::dec
#else
#define STREAM_HEX(x) cometos::hex << (x) << cometos::dec
#endif

#define recordScalar(x,y)

#define ENTER_METHOD_SILENT()
#define TAKE_MESSAGE(x)

#define CANCEL_AND_DELETE(x) delete (x)

namespace cometos {

void initialize();

void run();

void stop();

/**currently this log level is used if printing log message outsid a module
 */
void setRootLogLevel(uint8_t level);


bool run_once();


}

#endif /* COMETOS_H_ */
