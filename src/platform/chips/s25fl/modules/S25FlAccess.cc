
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

#include "S25FlAccess.h"
#include "s25fl_blocking.h"

S25FlAccess::S25FlAccess(const char * name) :
    cometos::RemoteModule(name)
{}

void S25FlAccess::initialize() {
    RemoteModule::initialize();
    cometos_error_t res = s25fl_init();
    ASSERT(res == COMETOS_SUCCESS);

    remoteDeclare(&S25FlAccess::read, "fr");
    remoteDeclare(&S25FlAccess::write, "fw");
    remoteDeclare(&S25FlAccess::erase, "fe");
}

cometos::Vector<uint8_t, FLASH_DATA_MAX_SIZE> S25FlAccess::read(S25FlAccessMsg & cfg) {
    cometos_error_t result = COMETOS_ERROR_FAIL;
    cometos::Vector<uint8_t, FLASH_DATA_MAX_SIZE> tmp;
    if (cfg.len <= FLASH_DATA_MAX_SIZE) {
        result = s25fl_read(cfg.addr, tmp.getBuffer(), cfg.len);
        tmp.setSize(cfg.len);
    } else {
        ASSERT(false);
    }

    ASSERT(result == COMETOS_SUCCESS);
    return tmp;
}

bool S25FlAccess::write(S25FlWriteMsg & cfg) {
    cometos_error_t result;
    result = s25fl_pageProgram(cfg.addr, cfg.data.getBuffer(), cfg.data.getSize());
    return result == COMETOS_SUCCESS;
}

bool S25FlAccess::erase(S25FlAccessMsg & cfg) {
    cometos_error_t result = s25fl_sectorErase(cfg.addr);
    return result == COMETOS_SUCCESS;
}
