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
 * @author Stefan Unterschütz
 */

#include "cometos.h"
#include "Platform.h"


#include "pal.h"

uint16_t intrand(uint16_t r)
{
   return Platform::intrand(r);
}

extern "C" {
int atexit(void (*func)(void)) {
    return 0;
}
}


namespace cometos {
void initialize() {
    pal_init();
    Module::initializeAll();
}

void run() {
    getScheduler().run(true);
}
bool run_once() {
    return getScheduler().run(false);
}

// returns task scheduler
TaskScheduler &getScheduler() {
    static TaskScheduler scheduler;
    return scheduler;
}

void stop() {
    getScheduler().stop();
}

static uint8_t _level = LOG_LEVEL_FATAL;
void setRootLogLevel(uint8_t level) {
    _level = level;
}

} // namespace cometos



/* the following is in the default namespace because we never now where our
 * "call" of a logging function, which actually is only a define macro
 * may originate
 */
#ifdef ENABLE_LOGGING

#include "logging.h"
uint8_t logging_getLevel() {
    const cometos::Module* mod = cometos::getScheduler().currentContext();
    if (mod == NULL) {
        return cometos::_level;
    }

    uint8_t tmp = cometos::getScheduler().getCurrentLogLevel();
    if (tmp == LOG_LEVEL_INVALID) {
        return cometos::_level;
    } else {
        return tmp;
    }
}

const char * getName() {
    const cometos::Module* mod = cometos::getScheduler().currentContext();
    if (mod != NULL) {
        return mod->getName();
    }
    return "root";
}

#endif


