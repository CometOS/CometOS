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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <omnetpp.h>
#include "simUtil.h"
#include "OutputStream.h"

namespace cometos {

std::ostream& operator<<(std::ostream& lhs, const cometos::Hex& rhs);
std::ostream& operator<<(std::ostream& lhs, const cometos::Dec& rhs);
std::ostream& operator<<(std::ostream& lhs, const cometos::Endl& rhs);

}

#ifdef ENABLE_LOGGING
#include "Logger.h"
#include "palId.h"
#include <iostream>

#define HEXOUT std::hex
#define DECOUT std::dec

#define ASSERT_DBG(x,u) ASSERT(x)

#ifndef GTEST_BUILD

inline void cometos_logRaw(uint8_t level, node_t channel, const char* str) {
    std::stringstream ss; ss.precision(9);
    ss << str;
    getLogger().log(getName(), channel, level, ss.str());
}

#define PREFIX omnetpp::simTime().dbl()<<"|"<<getFullName()<<"|"<<__func__<<"|"

#define LOG(level,channel,msg) { \
    std::stringstream ss; ss.precision(9);\
    ss << PREFIX; \
    getLogger().log(getName(), channel , level, ss.str()); \
}

#define LOG_PREFIX(level, channel) {\
    std::stringstream ss;\
    ss << PREFIX;\
    getLogger().log(getName(), channel, level, ss.str());\
}

#define LOG_RAW(level, channel, msg) {\
    std::stringstream ss;\
    ss << msg; \
    getLogger().log(getName(), channel, level, ss.str());\
}

#define LOG_FATAL(msg) LOG(0,palId_id(),msg)
#define LOG_ERROR(msg) LOG(1,palId_id(),msg)
#define LOG_WARN(msg) LOG(2,palId_id(),msg)
#define LOG_INFO(msg) LOG(3,palId_id(),msg)
#define LOG_INFO_PURE(msg) LOG_RAW(3,palId_id(), msg)
#define LOG_INFO_PREFIX  LOG_PREFIX(3, palId_id())
#define LOG_ENDL cometos::endl
#define LOG_DEBUG(msg) LOG(4,palId_id(),msg)
#define LOG_DEBUG_PURE(msg) LOG_RAW(4,palId_id(), msg)
#define LOG_DEBUG_PREFIX  LOG_PREFIX(4, palId_id())

#else // GTEST_BUILD

#define GTEST_PREFIX "[--LOG-----] "

#define LOG(msg) { \
    std::cout << GTEST_PREFIX << msg << std::endl;\
}

#define LOG_PREFIX {\
    std::cout << GTEST_PREFIX;\
}

#define LOG_RAW(msg) {\
    std::cout << msg;\
}

#define LOG_FATAL(msg) LOG(msg)
#define LOG_ERROR(msg) LOG(msg)
#define LOG_WARN(msg) LOG(msg)
#define LOG_INFO(msg) LOG(msg)
#define LOG_INFO_PURE(msg) LOG_RAW(msg)
#define LOG_INFO_PREFIX LOG_PREFIX
#define LOG_DEBUG(msg) LOG(msg)
#define LOG_DEBUG_PURE(msg) LOG_RAW(msg)
#define LOG_DEBUG_PREFIX LOG_PREFIX

#endif // GTEST_BUILD

#else // ENABLE_LOGGING

#define LOG_FATAL(msg)
#define LOG_ERROR(msg)
#define LOG_WARN(msg)
#define LOG_INFO(msg)
#define LOG_INFO_PURE(msg)
#define LOG_INFO_PREFIX
#define LOG_DEBUG(msg)
#define LOG_DEBUG_PURE(msg)
#define LOG_DEBUG_LAYER(msg, layer)
#define LOG_DEBUG_PREFIX

#endif


#endif
