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

#include "cometos.h"
#include "palLed.h"
#include "Logger.h"
#include "logging.h"
#include "s25fl_blocking.h"
#include "palId.h"
#include "OutputStream.h"
#include "cfs-coffee.h"
#include "arbiter/Resource.h"
#include "cfsSPIUser.h"
#include <util/delay.h>
#include "Module.h"
#include "Alive.h"
#include "TimeSyncService.h"
#include "SystemMonitor.h"

int main() {
    cometos::getCout() << "Starting cfs-logger test program" << cometos::endl;
    cometos::initialize();
    cometos::run();
}


using namespace cometos;

void writeResultsToUart();

const char log_conf[] = "log.conf";
static const char writeBuf[BUFSIZE] = "  *, debug,\t\r file\t.log, *\n *, info, stdout, *";
static char readBuf[BUFSIZE];
static char readLogBuf[BUFSIZE];

//static TimeSyncService tss;
//static Heartbeat wd("bla");
//static SystemMonitor sys;

class LoggerCFSTest;

void cfsCb(cometos_error_t result, int retVal);
void formatCb(cometos_error_t result, int retVal);
void loggerCb(cometos_error_t result, const char* file);

LoggerCFSTest* &getInstance() {
    static LoggerCFSTest* lcfst = NULL;
    return lcfst;
}

class LoggerCFSTest : public cometos::Module {
public:
    static const uint8_t OPENTOWRITE = 8;
    static const uint8_t OPENTOREAD = 6;
    static const uint8_t SEEK = 4;
    static const uint8_t READ = 1;
    static const uint8_t WRITE = 3;
    static const uint8_t CLOSE = 5;
    static const uint8_t UART = 7;

    static const uint8_t LOG_CONF = 1;
    static const uint8_t LOG_WRITE = 2;
    static const uint8_t LOG_DONE = 3;
    static const uint8_t LOG_UNSPEC = 4;

    LoggerCFSTest() :
        result(COMETOS_SUCCESS),
        mode(OPENTOWRITE|UART),
        spiResource(&user)
    {}

    virtual void initialize() {
        palLed_init();
        user.init();

        //user.format(formatCb);

        getInstance() = this;
        isConf = false;
        currLogOp = LOG_UNSPEC;
        if(mode & OPENTOWRITE) {
            currOp = OPENTOWRITE;
            schedule(&doStuff, &LoggerCFSTest::doOpen, 0);
        } else if (mode & OPENTOREAD) {
            currOp = OPENTOREAD;
            schedule(&doStuff, &LoggerCFSTest::doOpen, 0);
        }else if (mode & CLOSE) {
            currOp = CLOSE;
            schedule(&doStuff, &LoggerCFSTest::doClose, 0);
        }else if (mode & READ) {
            currOp = READ;
            schedule(&doStuff, &LoggerCFSTest::doRead, 0);
        }else if (mode & WRITE) {
            currOp = WRITE;
            schedule(&doStuff, &LoggerCFSTest::doWrite, 0);
        } else if (mode & SEEK) {
            currOp = SEEK;
            schedule(&doStuff, &LoggerCFSTest::doSeek, 0);
        }
    }

    void error(uint8_t val) {
        cometos::getCout() << "error" << cometos::endl;
        while(1) {
            switch(val) {
            case 1:_delay_ms(100); break;
            case 2:_delay_ms(500); break;
            case 3:_delay_ms(1000); break;
            }
            palLed_toggle(4);
        }
    }

    void cfsCallback(cometos_error_t result, int retVal) {
        if (result != COMETOS_SUCCESS) {
            if (result == COMETOS_ERROR_BUSY) {
                error(2);
            } else {
                cometos::getCout() << "cfsCallback: COMETOS_ERROR_FAIL" << cometos::endl;
                error(3);
            }
        }
        cometos::getCout() << "op=" << (int) currOp  << "|r=" << (int) result << "|mode="<< mode << cometos::endl;
        switch (currOp) {
        case OPENTOWRITE:{
            fd = retVal;
            if (mode & SEEK) {
                currOp = SEEK;
                schedule(&doStuff, &LoggerCFSTest::doSeek, 0);
            }
        } break;
        case OPENTOREAD:{
            fd = retVal;
            if (mode & READ) {
                currOp = READ;
                schedule(&doStuff, &LoggerCFSTest::doRead, 0);
            }
        } break;
        case SEEK:{
            cometos::getCout() << "newPosInFile=" << retVal << cometos::endl;
            if (mode & WRITE) {
                currOp = WRITE;
                schedule(&doStuff, &LoggerCFSTest::doWrite, 0);
            }
        } break;
        case WRITE: {
            cometos::getCout() << "nOfWrittenBytes=" << retVal << cometos::endl;
            if (mode & CLOSE) {
                currOp = CLOSE;
                schedule(&doStuff, &LoggerCFSTest::doClose, 0);
            }
        } break;
        case CLOSE: {
            if (mode & OPENTOREAD) {
                currOp = OPENTOREAD;
                mode = READ;
                schedule(&doStuff, &LoggerCFSTest::doOpen, 0);
            } else {
                currOp = UART;
                currLogOp = LOG_CONF;
                schedule(&readDone, &LoggerCFSTest::writeResultsToUart, 0);
            }
        } break;
        case READ: {
            cometos::getCout() << "nOfReadBytes=" << retVal << cometos::endl;
            if (mode & CLOSE) {
                currOp = CLOSE;
                schedule(&doStuff, &LoggerCFSTest::doClose, 0);
            }
        }
        }
    }

    void loggerCallback(cometos_error_t result, const char* file) {
        if (result != COMETOS_SUCCESS) {
            if (result == COMETOS_ERROR_BUSY) {
                error(2);
            } else {
                error(3);
            }
        }
        switch (currLogOp) {
        case LOG_CONF: {
            currLogOp = LOG_WRITE;
            schedule(&writeLog, &LoggerCFSTest::callLogger, 0);
        } break;
        case LOG_WRITE: {
            currLogOp = LOG_DONE;
            schedule(&writeLog, &LoggerCFSTest::callLogger, 0);
        } break;
        case LOG_DONE: {
            cometos::getCout() << "Logging successful" << cometos::endl << cometos::endl;
            currOp = OPENTOREAD;
            mode = READ;
            currFile = file;
            cometos::getCout() << "Main: currFile=" << currFile << cometos::endl;
            if (currFile != NULL) {
                schedule(&doStuff, &LoggerCFSTest::doOpen, 0);
            }
        } break;
        }
    }

    void formatCallback(cometos_error_t result, int retVal) {
        if (result != COMETOS_SUCCESS){
            error(1);
        }
        if (retVal != -1) {
            cometos::getCout() << "format successful" << cometos::endl;
        }
    }

    void doOpen(cometos::Message* msg) {
        _delay_ms(1000);
        if (currOp == OPENTOWRITE) {
            result = user.open(log_conf,CFS_WRITE, cfsCb);
        } else if (currOp == OPENTOREAD) {
            if (currLogOp != LOG_DONE) {
                result = user.open(log_conf,CFS_READ, cfsCb);
            } else {
                result = user.open(currFile,CFS_READ, cfsCb);
                result = user.open("file.log_0",CFS_READ, cfsCb);
            }
        }
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doClose(cometos::Message* msg) {
        _delay_ms(1000);
        result = user.close(fd, cfsCb);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doRead(cometos::Message* msg) {
        _delay_ms(1000);
        if (currLogOp == LOG_DONE) {
            result = user.read(fd, readLogBuf, BUFSIZE, cfsCb);
        } else {
            result = user.read(fd, readBuf, BUFSIZE, cfsCb);
        }
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doWrite(cometos::Message* msg) {
        _delay_ms(1000);
        result = user.write(fd, writeBuf, BUFSIZE-offset, cfsCb);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doSeek(cometos::Message* msg) {
        _delay_ms(1000);
        result = user.seek(fd, offset, CFS_SEEK_CUR, cfsCb);
        if (result != COMETOS_SUCCESS){
            error(3);
        }
    }

    void writeResultsToUart(cometos::Message* msg) {
        cometos::getCout() << "Uart:" << cometos::endl;
        _delay_ms(1000);
        if (!isConf) {
            isConf = true;
            for (int i = 0; i < BUFSIZE; i++) {
                cometos::getCout() << readBuf[i] << " ";
            }
            cometos::getCout() << "" << cometos::endl;
#ifdef ENABLE_LOGGING
            getLogger().configureLogger(log_conf, loggerCb);
#endif
        } else {
            for (int i = 0; i < BUFSIZE; i++) {
                cometos::getCout() << readLogBuf[i] << " ";
            }
            cometos::getCout() << cometos::endl;
        }
    }


    void callLogger(cometos::Message* msg) {
        const char* name = "ALL";
        const char* message = "testMsgtestMsgtestMsgtestMsgtestMsgtestMsgtestMsgtestMsgtestMsg";
        int channel, level;

        if (currLogOp == LOG_WRITE) {
            channel = 0;
            level = 4;
        } else {
            channel = 0;
            level = 3;
        }
        getLogger().log(name, channel, level, message, loggerCb);
    }

private:
    uint8_t result;
    uint8_t mode;
    uint8_t currOp;
    uint8_t currLogOp;
    const char* currFile = NULL;
    int fd;
    uint8_t offset = 0;
    bool isConf;

    cometos::Message doStuff;
    cometos::Message readDone;
    cometos::Message writeLog;

    cfsSPIUser user;
    Resource<1> spiResource;
};



void cfsCb(cometos_error_t result, int retVal) {
    getInstance()->cfsCallback(result, retVal);
}

void formatCb(cometos_error_t result, int retVal) {
    getInstance()->formatCallback(result, retVal);
}

void loggerCb(cometos_error_t result, const char* file) {
    getInstance()->loggerCallback(result, file);
}


static LoggerCFSTest lcfst;

