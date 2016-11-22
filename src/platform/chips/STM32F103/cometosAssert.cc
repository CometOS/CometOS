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

#include "logging.h"
#include "palFirmware.h"
#include <string.h>
#include "palLed.h"
#include "palPers.h"
#include "palExec.h"
#include "palExecUtils.h"
#include "OutputStream.h"

#include "cometosAssert.h"

void setBlockingCout(bool value); // defined in pal.cc

//#include <unwind.h> // GCC's internal unwinder, part of libgcc
//_Unwind_Reason_Code trace_fcn(_Unwind_Context *ctx, void *d)
//{
//#ifdef SERIAL_ASSERT
//	//commented that out because I suspect it to cause a crash!
//	/*int *depth = (int*)d;
//    cometos::getCout() << cometos::dec << (uint16_t)*depth << ": program counter at 0x" << cometos::hex << (uint32_t)_Unwind_GetIP(ctx) << cometos::endl;
//    (*depth)++;
//
//    */
//	return _URC_NO_REASON;
//	cometos::getCout() << "trace_fcn called";
//#endif
//}

void resetTheNode() {
    asm volatile ("cpsid i");
//    int depth = 0;
    //_Unwind_Backtrace(&trace_fcn, &depth);

    for(uint32_t i = 0; i < 50; i++) {
       palLed_toggle(i); 
       for(uint32_t j = 0; j < 3000000; j++) {
           __asm__ volatile ( "nop" );
       }
    }

    palExec_reset();
}

#ifdef ASSERT_SHORT
    #ifdef SERIAL_ASSERT
    void doAssert(uint16_t line, uint16_t fileId) {
        setBlockingCout(true);
        cometos::getCout() << "A:" << cometos::dec << fileId << ":" << line << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert(uint16_t line, uint16_t fileId) {
        setBlockingCout(true);
        palExec_writeAssertInfoShort(line, fileId);
        resetTheNode();
    }
    #endif
#else

#ifdef ASSERT_LONG
    #ifdef SERIAL_ASSERT
    void doAssert(uint16_t line, const char* filename) {
        setBlockingCout(true);
        palLed_on(4);
    	cometos::getCout() << "A:" << cometos::dec << line << ":" << filename  << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert(uint16_t line, const char* filename) {
        setBlockingCout(true);
        palExec_writeAssertInfoLong(line, filename);
        resetTheNode();
    }
    #endif
#else

    #ifdef SERIAL_ASSERT
    void doAssert() {
        setBlockingCout(true);
        cometos::getCout() << "A:" << cometos::dec << cometos::endl;
        resetTheNode();
    }
    #else
    void doAssert() {
        setBlockingCout(true);
    	palLed_on(1);
        resetTheNode();
    }
    #endif

#endif
#endif

