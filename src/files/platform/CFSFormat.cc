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

#include "CFSFormat.h"
#include "palWdt.h"
#include "cfs.h"

namespace cometos {

const char* const CFSFormat::MODULE_NAME = "fsf";

CFSFormat::CFSFormat() :
    cometos::RemoteModule(MODULE_NAME),
    formatAsyncDone(this, "faD")
{}

void CFSFormat::initialize() {
    // TODO could/should be transformed into a async mechanism
    //      to prevent looooong waiting times
    cometos::RemoteModule::initialize();
    remoteDeclare(&CFSFormat::format, "f");
    remoteDeclare(&CFSFormat::formatAsync, "fa", "faD");
}

uint8_t CFSFormat::format()
{
#ifndef OMNETPP
    palLed_on(2);
    palWdt_pause();
    uint8_t retval = cfs_format();
    palWdt_resume();
    palLed_off(2);
    return retval;
#else
    return 0;
#endif
}

void CFSFormat::formatAsync()
{
    // wait for 2 seconds before blocking the node, to allow for an remote access answer
    schedule(new Message(), &CFSFormat::doFormatAsync, 2000);
}

void CFSFormat::doFormatAsync(Message* msg) {
    delete(msg);
    uint8_t retval = format();
    formatAsyncDone.raiseEvent(retval);
}

}
