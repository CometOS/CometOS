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

#include "OutputStream.h"

namespace cometos {

#if 0
//#if defined OMNETPP or defined BOARD_local or defined BOARD_python
std::ostream& hex(std::ostream& os) {
    return os << std::hex;
}

std::ostream& dec(std::ostream& os) {
    return os << std::dec;
}

std::ostream& endl(std::ostream& os) {
    return os << std::endl;
}

std::ostream& getCout() {
    return std::cout;
}

void OutputStream::flush(Event* event) {
    flush();

    if(event != nullptr) {
        events.push(event);
        events.scheduleAllEvents();
    }
}

//#else
#endif

Endl endl;
Hex hex;
Dec dec;


void devnull(char c) {
    // nothing.
}

OutputStream& __attribute__((weak)) getCout() {
    static OutputStream nullstream(devnull);
    return nullstream;
}

void cometos::OutputStream::uitoa(uint64_t val) {
    char c;
    char s[20];
    int i = 0;

    do {
        c = (char) (val % this->radix);
        if (c >= 10)
            c += ('A' - 10);
        else
            c += '0';
        s[i++] = c;
        val = val / this->radix;
    } while (val);

    // fill characters
    for(uint8_t j = 0; j < wide - i; j++) {
        this->printChar(fillch);
    }
    
    // print number
    do {
        this->printChar(s[--i]);
    } while (i);
}

void cometos::OutputStream::itoa(int64_t val) {
    if (val < 0) {
        this->printChar('-');
        val = -val;
    }
    uitoa(val);
}

cometos::OutputStream::OutputStream(putchar_t putchar, flush_t flushfun) :
    printChar(putchar), flushfun(flushfun), radix(10), wide(0), fillch('0') {
}

cometos::OutputStream& cometos::OutputStream::operator<<(const uint8_t& data) {
    this->printChar((char)data);
    return *this;
}


cometos::OutputStream& cometos::OutputStream::operator<<(const int8_t& data) {
    this->printChar((char)data);
    return *this;
}


cometos::OutputStream& cometos::OutputStream::operator<<(const char* data) {
    while (*data)
        this->printChar(*(data++));
    return *this;
}

cometos::OutputStream& cometos::OutputStream::operator<<(char* const& data) {
    const char* copy = data;
    return (operator<<(copy));
}

cometos::OutputStream& cometos::OutputStream::operator<<(const Endl& data) {
    this->printChar('\n');
    return *this;
}


cometos::OutputStream& cometos::OutputStream::operator<<(const bool & data) {
    if (data) {
        this->printChar('t');
    } else {
        this->printChar('f');
    }
    return *this;
}


cometos::OutputStream& cometos::OutputStream::operator<<(const Hex& data) {
    radix = 16;
    return *this;
}

cometos::OutputStream& cometos::OutputStream::operator<<(const Dec& data) {
    radix = 10;
    return *this;
}

void cometos::OutputStream::fill(char fillch) {
    this->fillch = fillch;
}

void cometos::OutputStream::width(uint8_t wide) {
    this->wide = wide;
}

void cometos::OutputStream::flush(Event* event) {
    if(flushfun != 0) {
        flushfun(event);
    }
    else if(event != nullptr) {
        events.push(event);
        events.scheduleAllEvents();
    }
}

//#endif

} // namespace cometos
