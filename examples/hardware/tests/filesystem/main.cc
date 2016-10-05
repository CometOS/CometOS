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

/**@file Testing TWI/EEPRM in Interrupt Mode
 * @author Andreas Weigel
 */
#include "cometos.h"
#include "logging.h"
#include "s25fl_blocking.h"
//#include "S25FlAccess.h"
//#include "protocolstack.h"
#include "palLed.h"
#include "palId.h"
#include "OutputStream.h"
#include "cfs-coffee.h"
#include "arbiter/Resource.h"
#include "cfsSPIUser.h"
#include <util/delay.h>
#include "Module.h"


using namespace cometos;

void writeResultsToUart();

#define BUFSIZE 10
static const char buf[BUFSIZE] = "abcd12345";
static char rxBuf[BUFSIZE];

class AsyncCFSTest;

void cfsCb(cometos_error_t result, int retVal);
void formatCb(cometos_error_t result, int retVal);

AsyncCFSTest* &getInstance() {
    static AsyncCFSTest* acfst = NULL;
    return acfst;
}

class AsyncCFSTest : public cometos::Module {
public:
    static const uint8_t OPENTOWRITE = 8;
    static const uint8_t OPENTOREAD = 6;
    static const uint8_t SEEK = 4;
    static const uint8_t READ = 1;
    static const uint8_t WRITE = 3;
    static const uint8_t CLOSE = 5;
    static const uint8_t UART = 7;

    AsyncCFSTest() :
        result(COMETOS_SUCCESS),
        mode(OPENTOWRITE|UART),
        spiResource(&user)
    {}

    virtual void initialize() {
        palLed_init();
        user.init();

        //user.format(formatCb);

        getInstance() = this;
        if(mode & OPENTOWRITE) {
            currOp = OPENTOWRITE;
            schedule(&doStuff, &AsyncCFSTest::doOpen, 0);
        } else if (mode & OPENTOREAD) {
            currOp = OPENTOREAD;
            schedule(&doStuff, &AsyncCFSTest::doOpen, 0);
        }else if (mode & CLOSE) {
            currOp = CLOSE;
            schedule(&doStuff, &AsyncCFSTest::doClose, 0);
        }else if (mode & READ) {
            currOp = READ;
            schedule(&doStuff, &AsyncCFSTest::doRead, 0);
        }else if (mode & WRITE) {
            currOp = WRITE;
            schedule(&doStuff, &AsyncCFSTest::doWrite, 0);
        } else if (mode & SEEK) {
            currOp = SEEK;
            schedule(&doStuff, &AsyncCFSTest::doSeek, 0);
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
                error(3);
            }
        }
        cometos::getCout() << "op=" << (int) currOp  << "|r=" << (int) result << "|mode="<< mode << cometos::endl;
        switch (currOp) {
        case OPENTOWRITE:{
            fd = retVal;
            if (mode & SEEK) {
                currOp = SEEK;
                schedule(&doStuff, &AsyncCFSTest::doSeek, 0);
            }
        } break;
        case OPENTOREAD:{
            fd = retVal;
            if (mode & READ) {
                currOp = READ;
                schedule(&doStuff, &AsyncCFSTest::doRead, 0);
            }
        } break;
        case SEEK:{
            cometos::getCout() << "newPosInFile=" << retVal << cometos::endl;
            if (mode & WRITE) {
                currOp = WRITE;
                schedule(&doStuff, &AsyncCFSTest::doWrite, 0);
            }
        } break;
        case WRITE: {
            cometos::getCout() << "nOfWrittenBytes=" << retVal << cometos::endl;
            if (mode & CLOSE) {
                currOp = CLOSE;
                schedule(&doStuff, &AsyncCFSTest::doClose, 0);
            }
        } break;
        case CLOSE: {
            if (mode & OPENTOREAD) {
                currOp = OPENTOREAD;
                mode = READ;
                schedule(&doStuff, &AsyncCFSTest::doOpen, 0);
            } else {
                currOp = UART;
                schedule(&readDone, &AsyncCFSTest::writeResultsToUart, 0);
            }
        } break;
        case READ: {
            cometos::getCout() << "nOfReadBytes=" << retVal << cometos::endl;
            if (mode & CLOSE) {
                currOp = CLOSE;
                schedule(&doStuff, &AsyncCFSTest::doClose, 0);
            }
        }
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
            result = user.open("test",CFS_WRITE, cfsCb);
        } else if (currOp == OPENTOREAD) {
            result = user.open("test",CFS_READ, cfsCb);
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
        result = user.read(fd, rxBuf, BUFSIZE, cfsCb);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doWrite(cometos::Message* msg) {
        _delay_ms(1000);
        result = user.write(fd, buf, 4, cfsCb);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doSeek(cometos::Message* msg) {
        _delay_ms(1000);
        result = user.seek(fd, 5, CFS_SEEK_CUR, cfsCb);
        if (result != COMETOS_SUCCESS){
            error(3);
        }
    }

    void writeResultsToUart(cometos::Message* msg) {
        _delay_ms(1000);
        cometos::getCout() << "Uart:" << cometos::endl;
        for (int i = 0; i < BUFSIZE; i++) {
            cometos::getCout() << rxBuf[i] << " ";
        }
        cometos::getCout() << "" << cometos::endl;
        for (int i = 0; i < BUFSIZE; i++) {
            cometos::getCout() << buf[i] << " ";
        }
    }

private:
    uint8_t result;
    uint8_t mode;
    uint8_t currOp;
    int fd;

    cometos::Message readDone;
    cometos::Message doStuff;

    cfsSPIUser user;
    Resource<1> spiResource;

    //const char buf[BUFSIZE] = "abcd12345";
    //char rxBuf[BUFSIZE];
};



void cfsCb(cometos_error_t result, int retVal) {
    getInstance()->cfsCallback(result, retVal);
}

void formatCb(cometos_error_t result, int retVal) {
    getInstance()->formatCallback(result, retVal);
}

static AsyncCFSTest acfst;


int main() {
    cometos::getCout() << "Starting async cfs test program" << cometos::endl;

    cometos::initialize();
    cometos::getCout() << "After initialize" << cometos::endl;
    cometos::run();
}



/**
    //static S25FlAccess ft;
    static const char buf[10] = "abcd12345";
    static char rxBuf[10];

    int main() {
        palLed_init();
        //protocolstack_init();
        cometos::initialize();

        _delay_ms(1000);
        int fileD;
        fileD = cfs_open("test", CFS_WRITE);
        cfs_seek(fileD, 5, CFS_SEEK_CUR);
        cfs_write(fileD, buf, 4);
        cfs_close(fileD);
        fileD = cfs_open("test", CFS_READ);
        cfs_read(fileD, rxBuf, 10);
        cfs_close(fileD);

        for (int i = 0; i < 10; i++) {
            cometos::getCout() << rxBuf[i] << " ";
        }

        cometos::getCout() << cometos::endl;
        cometos::run();
        return 0;
    }**/
