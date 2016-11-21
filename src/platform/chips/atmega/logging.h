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

/**In this file CometOS logging interface is declared. Logging is
 * still work in progress and not finished yet.
 *
 * @author Stefan Unterschuetz
 */

#ifndef LOGGING_H_
#define LOGGING_H_

// THIS FILE DEFINES HOW ENABLE_LOGGING MESSAGES AND ASSERTS ARE HANDLED

#include "logLevels.h"
#include <stdint.h>

#ifdef ENABLE_LOGGING
#include "OutputStream.h"
#include "palLocalTime.h"
#include "NetworkTime.h"

#define HEXOUT cometos::hex
#define DECOUT cometos::dec

/**@return 	current log level, various implementation of this function are possible,
 * 			in case of CometOS, different log levels can be assigned to modules */
uint8_t logging_getLevel();
const char* getName();

#define LOG(level,msg) {if (level>=logging_getLevel()) cometos::getCout()<<::NetworkTime::get()<<"|"<<getName()<<"|"<<msg<<"\n";}
#define LOG_PREFIX(level) {if (level>=logging_getLevel()) cometos::getCout()<<::NetworkTime::get()<<"|"<<getName()<<"|";}
#define LOG_RAW(level,msg) {if (level>=logging_getLevel()) cometos::getCout()<<msg;}

#ifdef LOGGING_DEBUG
#define LOGGING_INFO
#define LOG_DEBUG(msg) LOG( LOG_LEVEL_DEBUG , "D:"<<msg)
#define LOG_DEBUG_PURE(msg) LOG_RAW( LOG_LEVEL_DEBUG , msg)
#define LOG_DEBUG_PREFIX LOG_PREFIX( LOG_LEVEL_DEBUG )
#else
#define LOG_DEBUG(msg)
#define LOG_DEBUG_PURE(msg)
#define LOG_DEBUG_RAW(msg)
#define LOG_DEBUG_PREFIX
#endif

#ifdef LOGGING_INFO
#define LOGGING_WARN
#define LOG_INFO(msg) LOG( LOG_LEVEL_INFO , "I:"<<msg)
#define LOG_INFO_PURE(msg) LOG_RAW( LOG_LEVEL_INFO , msg)
#define LOG_INFO_PREFIX LOG_PREFIX( LOG_LEVEL_INFO )
#else
#define LOG_INFO(msg)
#define LOG_INFO_PURE(msg)
#define LOG_INFO_PREFIX
#endif

#ifdef LOGGING_WARN
#define LOGGING_ERROR
#define LOG_WARN(msg) LOG( LOG_LEVEL_WARN , "W:"<<msg)
#else
#define LOG_WARN(msg)
#endif

#ifdef LOGGING_ERROR
#define LOG_ERROR(msg) LOG( LOG_LEVEL_ERROR , "E:"<<msg)
#else
#define LOG_ERROR(msg)
#endif

#define LOG_FATAL(msg) LOG( LOG_LEVEL_FATAL , "F:"<<msg)


#else

#define LOG_FATAL(msg)
#define LOG_ERROR(msg)
#define LOG_WARN(msg)
#define LOG_INFO(msg)
#define LOG_INFO_PURE(msg)
#define LOG_INFO_PREFIX
#define LOG_DEBUG(msg)
#define LOG_DEBUG_PURE(msg)
#define LOG_DEBUG_RAW(msg)
#define LOG_DEBUG_PREFIX

#endif

#endif /* LOGGING_H_ */
