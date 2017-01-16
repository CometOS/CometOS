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

#define HEXOUT std::hex
#define DECOUT std::dec

#include "logLevels.h"
#include <iostream>
#include "Logger.h"
#include "OutputStream.h"

#if 0
#include "assert.h"

#define ASSERT(x) if (!(x)) {std::cout << "ASSERT " << __FILE__ << ":" <<  __LINE__ << std::endl; }
#define ASSERT2(x,u) if (!(x)) {std::cout << "ASSERT " << __FILE__ << ":" <<  __LINE__ << std::endl; }
#define ASSERT_DBG(x,u) if (!(x)) {std::cout << "ASSERT " << __FILE__<< ":" << __LINE__; << std::endl;}
#endif

#include <sstream>
#include "NetworkTime.h"

#ifdef ENABLE_LOGGING

/* forward declaration (pretty ugly) */
namespace cometos {
    class Hex;
    class Dec;
    class Endl;
}

std::ostream& operator<<(std::ostream& lhs, const cometos::Hex& rhs);
std::ostream& operator<<(std::ostream& lhs, const cometos::Dec& rhs);
std::ostream& operator<<(std::ostream& lhs, const cometos::Endl& rhs);

const char * getName();

#define COMETOS_LOG_PREFIX(ss, level, channel) {\
    ss<<NetworkTime::get()<<"|"<<getName()<<"|"<<std::hex<<channel<<std::dec<<"|"<<__func__<<"|";\
}

#define LOG(level,channel,msg) { \
    std::stringstream ss; ss.precision(6);\
    COMETOS_LOG_PREFIX(ss, (level), (channel));\
    ss <<msg<<std::endl; \
    getLogger().log(getName(), (channel) , (level), ss.str()); \
}

#define LOG_RAW(level, channel, msg) {\
    std::stringstream ss;\
    ss << msg; \
    getLogger().log(getName(), channel, level, ss.str());\
}

#define LOG_PREFIX(level, channel) {\
    std::stringstream ss;\
    COMETOS_LOG_PREFIX(ss, level, channel);\
    getLogger().log(getName(), (channel), (level), ss.str());\
}

#define LOG_FATAL(msg) LOG(0,palId_id(),msg)
#define LOG_ERROR(msg) LOG(1,palId_id(),msg)
#define LOG_WARN(msg) LOG(2,palId_id(),msg)
#define LOG_INFO(msg) LOG(3,palId_id(),msg)
#define LOG_INFO_PURE(msg) LOG_RAW(4, palId_id(), msg)
#define LOG_INFO_PREFIX LOG_PREFIX(4, palId_id())
#define LOG_DEBUG(msg) LOG(4,palId_id(),msg)
#define LOG_DEBUG_PURE(msg) LOG_RAW(4, palId_id(), msg)
#define LOG_DEBUG_PREFIX LOG_PREFIX(4, palId_id())

#else

#define LOG_FATAL(msg)
#define LOG_ERROR(msg)
#define LOG_WARN(msg)
#define LOG_INFO(msg)
#define LOG_INFO_PURE(msg)
#define LOG_INFO_PREFIX
#define LOG_DEBUG(msg)
#define LOG_DEBUG_PURE(msg)
#define LOG_DEBUG_PREFIX

#endif

#define LOG_ENDL cometos::endl

#endif /* LOGGING_H_ */
