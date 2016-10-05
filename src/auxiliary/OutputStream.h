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

#ifndef OUTPUTSTREAM_H_
#define OUTPUTSTREAM_H_

#include "cometos.h"
#include "Event.h"

#include <stdint.h>

#if 0
//#if defined OMNETPP or defined BOARD_local or defined BOARD_python
#include <iostream>
namespace cometos {

class OutputStream : public std::ostream {
public:
    void flush(Event* event = nullptr);
};

std::ostream& hex(std::ostream& os);
std::ostream& dec(std::ostream& os);
std::ostream& endl(std::ostream& os);

OutputStream& getCout();

}

//#else
#endif

#include "TaskScheduler.h"


namespace cometos {

typedef void (*putchar_t)(char);
typedef void (*flush_t)(Event*);

/**End of line*/
class Endl {};
class Hex {};
class Dec {};

/**Class formated output with similar behaviour such as std::cout
 */
class OutputStream {
public:
    OutputStream(putchar_t putchar, flush_t flushfun = 0);

    // catch all integer types, except (u)int8_t, which print as characters
    template<class T>
    OutputStream& operator<<(const T& intval) {
       T val = intval;
       if (val < 0) {
           this->printChar('-');
           val = -val;
       }
       uitoa(val);
       return *this;
    }

    OutputStream& operator<<(const uint8_t& data);
    OutputStream& operator<<(const int8_t& data);
    OutputStream& operator<<(const char* data);
    OutputStream& operator<<(char* const& data);
    OutputStream& operator<<(const Endl& data);
    OutputStream& operator<<(const bool& data);
    OutputStream& operator<<(const Hex& data);
    OutputStream& operator<<(const Dec& data);

    void width(uint8_t wide);
    void fill(char fillch);

    void flush(Event* event = nullptr);

private:
    void uitoa(uint64_t val);
    void itoa(int64_t val);

    putchar_t printChar;
    flush_t flushfun;
    uint8_t radix;
    uint8_t wide;
    char fillch;

    EventQueue events;
};


OutputStream& getCout();
OutputStream& getSerialCout();


extern Endl endl;
extern Hex hex;
extern Dec dec;

}
//#endif

#endif /* OUTPUTSTREAM_H_ */
