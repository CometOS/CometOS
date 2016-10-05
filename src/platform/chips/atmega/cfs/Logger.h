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
#if 0

#ifndef LOGGER_H_
#define LOGGER_H_


#ifdef ENABLE_LOGGING
#include "cometos.h"
#include "cometosError.h"
#include "palId.h"
#include "Resource.h"
#include "cfsSPIUser.h"
#include "List.h"
#include "HashSet.h"
#include "Vector.h"
#include "charparser.h"
#include "logger_opcodes.h"

#define LOG_SET_SIZE 20
#define LOG_LIST_SIZE  20
#define LOG_VECTOR_SIZE  4
#define BUFSIZE 100
#define NAME_SIZE 15
#define FILE_SIZE 15


typedef HashSet<int, LOG_SET_SIZE> logNodeSet_t;

typedef struct LgDescriptor {
    char name[NAME_SIZE];
    bool logAll;
    bool useStdout;
    char file_0[FILE_SIZE];
    char file_1[FILE_SIZE];
    char file_2[FILE_SIZE];
    char activeFile[FILE_SIZE];
    logNodeSet_t nodes;
    int level;
} logDescriptor_t;

typedef List<logDescriptor_t*, LOG_LIST_SIZE> logDescriptorList_t;


class Logger {

public:
    Logger();
    virtual ~Logger();

    /**
     * Will be called when operation is done.
     */
    typedef void (*logger_callBack)(cometos_error_t status, const char* file);

    void configureLogger(const char log_conf[], logger_callBack cb);

    void log(const char* name, int channel, int priority,
            const char* message, logger_callBack cb);

    void openCallback(cometos_error_t result, int retVal);

    void readConfFileCallback(cometos_error_t result, int retVal);

    void seekCallback(cometos_error_t result, int retVal);

    void writeCallback(cometos_error_t result, int retVal);

    void rmCallback(cometos_error_t result, int retVal);

    /**
     * Reset all curr-values and call by user provided callback.
     */
    void callCallback(cometos_error_t result);

    void errorCallback(cometos_error_t result);

private:
    logDescriptorList_t loggers;
    logDescriptorList_t::iterator currIt;

    char rxBuf[BUFSIZE];
    const char* currMsg = NULL;
    char currFile[FILE_SIZE];
    int currFd = -2;
    log_op_t currOp = LOG_OP_UNSPEC;
    logger_callBack currCb = NULL;
    bool isConfigured;

    cfsSPIUser user;
    Resource<1> spiResource;

        void openConfFile(const char log_conf[]);

        void readConfFile(void);

        void configLogger(void);

        void getFileSize(void);

        void writeLog(int fileSize);
};


Logger &getLogger();

#endif /* ENABLE_LOGGING */

#endif /* LOGGER_H_ */
#endif
