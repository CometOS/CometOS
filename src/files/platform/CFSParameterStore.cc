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
 * @author Andreas Weigel
 */

#include "CFSParameterStore.h"
#include "AsyncAction.h"
#include "CFSArbiter.h"
#include "cfs.h"
#include "palLed.h"
#include "OutputStream.h"

namespace cometos {


const char* const CFSParameterStore::CONFIG_PREFIX = "cfg_";
const uint8_t CFSParameterStore::CONFIG_PREFIX_LEN = 4;

CFSParameterStore::CFSParameterStore() :
    ParameterStore() {

}

CFSParameterStore::~CFSParameterStore() {
}

cometos_error_t CFSParameterStore::storeRawData(Module * m, const StoredConfiguration & raw) {
    cometos_error_t result = getCFSArbiter()->requestImmediately();
    if(result != COMETOS_SUCCESS) {
        return result;
    }

    // derive filename to open from module
    char filename[CONFIG_PREFIX_LEN+MODULE_NAME_LENGTH];
    constructFilename(m, filename);

    int fd = cfs_open(filename, CFS_WRITE);
    if (fd < 0) {
        getCFSArbiter()->release();
        return COMETOS_ERROR_FAIL;
    }

    result = COMETOS_SUCCESS;

    int written = cfs_write(fd, raw.getConstBuffer(), raw.getSize());

    if (written != raw.getSize()) {
        result = COMETOS_ERROR_SIZE;
    }

    cfs_close(fd);

    getCFSArbiter()->release();
    return result;
}


cometos_error_t CFSParameterStore::readRawData(Module * m, StoredConfiguration & raw) {
    cometos_error_t result = getCFSArbiter()->requestImmediately();
    if(result != COMETOS_SUCCESS) {
        return result;
    }

    char filename[CONFIG_PREFIX_LEN+MODULE_NAME_LENGTH];
    constructFilename(m, filename);

    int fd = cfs_open(filename, CFS_READ);
    if (fd < 0) {
        // file not found
        getCFSArbiter()->release();
        return COMETOS_ERROR_NOT_FOUND;
    }

    result = COMETOS_SUCCESS;

    // get size of file and check if it fits into a StoredConfiguration array
    int filesize = cfs_seek(fd, 0, CFS_SEEK_END);
    if (filesize < 0) {
        result = COMETOS_ERROR_FAIL;
    }
    cfs_seek(fd, 0, CFS_SEEK_SET);

    if (filesize > raw.getMaxSize()) {
        result = COMETOS_ERROR_SIZE;
    }

    raw.clear();

    if (result == COMETOS_SUCCESS) {
        int read = cfs_read(fd, raw.getBuffer(), filesize);
        if (read != filesize) {
            result = COMETOS_ERROR_FAIL;
        }
        raw.setSize(read);
    }

    cfs_close(fd);

    getCFSArbiter()->release();
    return result;
}


cometos_error_t CFSParameterStore::resetRawData(Module * m) {
    cometos_error_t result = getCFSArbiter()->requestImmediately();
    if(result != COMETOS_SUCCESS) {
        return result;
    }

    char filename[CONFIG_PREFIX_LEN+MODULE_NAME_LENGTH];
    constructFilename(m, filename);

    int res = cfs_remove(filename);
    if (res < 0) {
        getCFSArbiter()->release();
        return COMETOS_ERROR_ALREADY;
    } else {
        getCFSArbiter()->release();
        return COMETOS_SUCCESS;
    }
}

inline void CFSParameterStore::constructFilename(Module * m, char * buf) {
    memcpy(buf, CONFIG_PREFIX, CONFIG_PREFIX_LEN);
    strncpy(buf + CONFIG_PREFIX_LEN, m->getName(), MODULE_NAME_LENGTH);
}


} /* namespace cometos */
