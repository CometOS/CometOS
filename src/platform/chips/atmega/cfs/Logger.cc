#if 0
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
 * @author Stefan Unterschuetz
 */

#include "Logger.h"

#ifdef ENABLE_LOGGING
#define NUM_LOG_LEVELS 	5
const char *LOG_LEVEL[NUM_LOG_LEVELS] = {"fatal", "error", "warn", "info", "debug"};


void openCb(cometos_error_t result, int retVal);
void readConfFileCb(cometos_error_t result, int retVal);
void seekCb(cometos_error_t result, int retVal);
void writeCb(cometos_error_t result, int retVal);
void rmCb(cometos_error_t result, int retVal);
void closeCb(cometos_error_t result, int retVal);
void dummyCb(cometos_error_t result, int retVal);


Logger::~Logger() {
    // delete all
    for (logDescriptorList_t::iterator it = loggers.begin(); it != loggers.end(); it++) {
        delete *it;
    }
}


Logger::Logger() : spiResource(&user) {
    // initialize SPI user
    user.init();
    isConfigured = false;
}


void Logger::configureLogger(const char log_conf[], logger_callBack cb) {
    currCb = cb;
    currOp = LOG_OP_CONF;
    openConfFile(log_conf);
}


void Logger::openConfFile(const char log_conf[]) {
    // open file log_conf for read operation
    cometos_error_t result = user.open(log_conf, CFS_READ, openCb);
    errorCallback(result);
}


void Logger::readConfFile(void) {
    cometos_error_t result = user.read(currFd, rxBuf, BUFSIZE, readConfFileCb);
    errorCallback(result);
}


void Logger::configLogger(void) {
    int line = 0;
    uint8_t offset = 0;
    // Retrieve Parameters as long as EoF not reached
    while(offset<BUFSIZE) {
        line++;
        char tmpBuf[strlen(rxBuf)];
        // Get line and check whether the end of the array was reached
        if(getLine(rxBuf, tmpBuf, offset)) {
            offset = BUFSIZE;
        }
        // 1 has to be added, since the \n char is omitted by getLine
        offset += strlen(tmpBuf)+1;

        //Remove spaces and other delimiters
        uint8_t nDelimiters = 4;
        char trimBuf[strlen(tmpBuf)];
        trimChArray(tmpBuf, trimBuf, " \t\r\n", nDelimiters);

        // Check whether the extracted and trimmed line is a comment
        if ((trimBuf[0]=='#') || (trimBuf[0]=='/' && trimBuf[1]=='/')) {
            continue;
        }

        // Extract config parameters and store each parameter as a vector element
        char* str = NULL;
        cometos::Vector<const char*,LOG_VECTOR_SIZE> tokens;
        str = strtok (trimBuf,",");
        while (str != NULL) {
            tokens.pushBack(str);
            str = strtok(NULL, ", ");
        }

        // check stuff
        if (tokens.getSize() < 4) {
            cometos::getCout() << "<error> missing arguments in line " << line << cometos::endl;
        }

        logDescriptor_t *descr = new logDescriptor_t;

        // read logger name
        if (strcmp(tokens[0],"*") == 0) {
            strcpy(descr->name,"ALL");
        } else {
            strcpy(descr->name,tokens[0]);
        }

        // read logger debug level
        int logLevel;
        for (logLevel = 0; logLevel < NUM_LOG_LEVELS; logLevel++) {
            if (strcmp(tokens[1],LOG_LEVEL[logLevel]) == 0) {
                break;
            }
        }
        descr->level = logLevel;

        if (logLevel == NUM_LOG_LEVELS) {
            cometos::getCout() << "<error> unknown log level for argument 2: " << line << cometos::endl;
            delete descr;
            continue;
        }

        // copy file name (output method) and add identifiers to the corresponding descriptor file names
        strcpy(descr->file_0,tokens[2]);
        strcpy(descr->file_1,tokens[2]);
        strcpy(descr->file_2,tokens[2]);
        strcat(descr->file_0,"_0");
        strcat(descr->file_1,"_1");
        strcat(descr->file_2,"_2");
        strcpy(descr->activeFile,"nn");
        if (strcmp(tokens[2],"stdout") == 0) {
            descr->useStdout = true;
        } else {
            descr->useStdout = false;
        }

        // if required, extract all nodes
        descr->logAll = false;
        if (strcmp(tokens[3],"*") == 0) {
            descr->logAll = true;
        } else {
            for (unsigned int i = 3; i < tokens.getSize(); i++) {
                if (strncmp(tokens[i],"0x",2) == 0) {
                    int value;
                    sscanf(tokens[i], "0x%x", &value);
                    descr->nodes.add(value);
                } else {
                    descr->nodes.add(atoi(tokens[i]));
                }
            }
        }
        loggers.push_back(descr);
    }

    cometos::getCout() << "Logger configured successfully " << cometos::endl << cometos::endl;
    isConfigured = true;
    cometos_error_t result = user.close(currFd, closeCb);
    errorCallback(result);
}


void Logger::writeLog(int fileSize) {
    cometos_error_t result = COMETOS_SUCCESS;
    // Check whether message can be stored in active log file
    if (fileSize+strlen(currMsg) > BUFSIZE) {
        // If not, close the active file and open the next one
        //result = user.close(currFd, dummyCb);
        errorCallback(result);
        if(strcmp((*currIt)->activeFile,(*currIt)->file_1)==0) {
            // File 1 full
            strcpy((*currIt)->activeFile,(*currIt)->file_2);
            cometos::getCout() << "new active file: " << (*currIt)->activeFile << cometos::endl;
            strcpy(currFile,(*currIt)->file_2);
            // Clear old file
            result = user.remove((*currIt)->file_0, rmCb);
        } else if(strcmp((*currIt)->activeFile,(*currIt)->file_2)==0) {
            // File 2 full
            strcpy((*currIt)->activeFile,(*currIt)->file_0);
            cometos::getCout() << "new active file: " << (*currIt)->activeFile << cometos::endl;
            strcpy(currFile,(*currIt)->file_0);
            // Clear old file
            result = user.remove((*currIt)->file_1, rmCb);
        } else {
            // File 0 full
            strcpy(currFile,(*currIt)->file_1);
            // Check whether file 0 has been used before
            if(strcmp((*currIt)->activeFile,(*currIt)->file_0)==0) {
                strcpy((*currIt)->activeFile,(*currIt)->file_1);
                cometos::getCout() << "new active file: " << (*currIt)->activeFile << cometos::endl;
                // Clear old file
                result = user.remove((*currIt)->file_2, rmCb);
            } else {
                // If not, the other two files are still empty, nothing should be deleted
                strcpy((*currIt)->activeFile,(*currIt)->file_1);
                cometos::getCout() << "new active file: " << (*currIt)->activeFile << cometos::endl;
                result = user.open(currFile, CFS_WRITE | CFS_APPEND | CFS_READ, openCb);
                errorCallback(result);
            }
        }
        errorCallback(result);
    } else {
        // Store message in active log file
        cometos::getCout() << "write; fileDescriptor: " << currFd << cometos::endl;
        result = user.write(currFd, currMsg, strlen(currMsg), writeCb);
        errorCallback(result);
    }
}


void Logger::getFileSize(void) {
    cometos::getCout() << "seek; fileDescriptor: " << currFd << cometos::endl;
    cometos_error_t result = user.seek(currFd, 0, CFS_SEEK_END, seekCb);
    errorCallback(result);
}


void Logger::log(const char* name, int channel, int priority, const char* message, logger_callBack cb) {
    if(!isConfigured) {
        cometos::getCout() << "<error> logger is not configured" << cometos::endl;
        return;
    }
    bool useCFS = false;
    currOp = LOG_OP_WRITE;
    currMsg = message;
    currCb = cb;

    // abort recursion if this was called with ALL, which is always done
    // once to check if some ALL target was defined
    if (strcmp(name,"ALL") != 0) {
        log("ALL", channel, priority, message, cb);
    }

    for (logDescriptorList_t::iterator it = loggers.begin(); it != loggers.end(); it++) {
        if (strcmp(name,(*it)->name)==0) {
            if (priority <= (*it)->level) {//==
                if ((*it)->logAll || (*it)->nodes.contains(channel)) {
                    if ((*it)->useStdout) {
                        //cometos::getCout() << "logger uses stdout" << cometos::endl;
                        //cout << message;
                        //cout.flush();
                    } else {
                        useCFS = true;
                        // If Logger::log is called for the first time, there is no current active file
                        if(strcmp((*it)->activeFile,"nn")==0) {
                            cometos::getCout() << "activeFile: " << (*it)->activeFile << cometos::endl;
                            strcpy(currFile,(*it)->file_0);
                        } else {
                            strcpy(currFile,(*it)->activeFile);
                        }
                        currIt = it;
                        cometos_error_t result = user.open(currFile, CFS_WRITE | CFS_APPEND | CFS_READ, openCb);
                        errorCallback(result);
                    }
                }
            }
        }
    }
    // Make sure that callback is called if stdout was used exclusively
    if (!useCFS) {
        callCallback(COMETOS_SUCCESS);
    }
}


static Logger logger;
Logger& getLogger() {
    return logger;
}


void Logger::openCallback(cometos_error_t result, int retVal) {
    errorCallback(result);
    // store file descriptor of opened file
    currFd = retVal;
    //call method depending on opcode
    if(currOp == LOG_OP_CONF) {
        readConfFile();
    } else {
        cometos::getCout() << "open file:  " << currFile << ", fileDescriptor: " << currFd << cometos::endl;
        getFileSize();
    }
}


void Logger::readConfFileCallback(cometos_error_t result, int retVal) {
    errorCallback(result);
    if (retVal != BUFSIZE) {
        cometos::getCout() << "warning: only " << retVal << " out of "
                << strlen(currMsg) << " bytes were read." << cometos::endl;
    }
    if(currOp == LOG_OP_CONF) {
        configLogger();
    } else {
        //DEBUG
        for (int i = 0; i < retVal; i++) {
            cometos::getCout() << rxBuf[i] << " ";
        }
        cometos::getCout() << "" << cometos::endl;
        callCallback(result);
        //result = user.close(currFd, closeCb);
        //errorCallback(result);
    }
}


void Logger::seekCallback(cometos_error_t result, int retVal) {
    cometos::getCout() << "fileSize=" << retVal << cometos::endl;
    errorCallback(result);
    writeLog(retVal);
}


void Logger::writeCallback(cometos_error_t result, int retVal) {
    errorCallback(result);
    if (retVal != strlen(currMsg)) {
        cometos::getCout() << "warning: only " << retVal << " out of "
                << strlen(currMsg) << " bytes were written." << cometos::endl;
    }
    //result = user.close(currFd, closeCb);
    cometos::getCout() << "read; fileDescriptor: " << currFd << cometos::endl;
    user.read(currFd, rxBuf, BUFSIZE, readConfFileCb);
    errorCallback(result);
}


void Logger::rmCallback(cometos_error_t result, int retVal) {
    errorCallback(result);
    if(retVal == 0) {
        result = user.open(currFile, CFS_WRITE | CFS_APPEND | CFS_READ, openCb);
        errorCallback(result);
    }
}


void Logger::callCallback(cometos_error_t result) {
    logger_callBack tmpCb = currCb;
    currCb = NULL;
    currIt = NULL;
    currOp = LOG_OP_UNSPEC;
    currFd = -2;
    currMsg = NULL;
    tmpCb(result, currFile);
    strcpy(currFile,"");
}


void Logger::errorCallback(cometos_error_t result) {
    if (result != COMETOS_SUCCESS) {
        cometos::getCout() << "error: " << result << cometos::endl;
        callCallback(result);
    }
}


void openCb(cometos_error_t result, int retVal) {
    getLogger().openCallback(result, retVal);
}

void readConfFileCb(cometos_error_t result, int retVal) {
    getLogger().readConfFileCallback(result, retVal);
}

void seekCb(cometos_error_t result, int retVal) {
    getLogger().seekCallback(result, retVal);
}

void writeCb(cometos_error_t result, int retVal) {
    getLogger().writeCallback(result, retVal);
}

void rmCb(cometos_error_t result, int retVal) {
    getLogger().rmCallback(result, retVal);
}

void closeCb(cometos_error_t result, int retVal) {
    getLogger().callCallback(result);
}

void dummyCb(cometos_error_t result, int retVal) {
    getLogger().errorCallback(result);
}


#endif

#endif
