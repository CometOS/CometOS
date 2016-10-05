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

/*
 * @author Martin Ringwelski
 */

#ifndef SSTRING_H_
#define SSTRING_H_

#include <cometos.h>
#include "primitives.h"
#include "logging.h"

namespace cometos {

enum SStringNumFormat {
    SString_DEC = 10,
    SString_HEX = 16,
    SString_OCT = 8,
    SString_BIN = 2
};

class SString_base {
public:
    SString_base(): _size(0) {}
    void clear() {
        _size = 0;
    }

    bool empty() const {
        return (_size == 0);
    }
    uint8_t size() const {
        return _size;
    }
    uint8_t length() const {
        return _size;
    }

protected:
    uint8_t _size;
};

template <uint8_t S>
class SString : public SString_base {
public:
    SString(): SString_base() {}
    SString(const char* str, uint8_t length = 0xFF): SString_base() {
        append(str, length);
    }
    template <uint8_t OS>
    SString(const SString<OS>& other): SString_base() {
        append(other);
    }
    virtual ~SString() {}

    bool full() const {
        return (_size == S);
    }

    uint8_t max_size() const {
        return S;
    }

    char& operator[](uint8_t pos) {
        ASSERT(pos < S);
        if (pos >= _size) {
            _size = (pos + 1);
        }
        return _str[pos];
    }

    char getChar(uint8_t pos) const {
        ASSERT(pos < _size);
        return _str[pos];
    }

    void append(char c) {
        if (_size < S) {
            _str[_size++] = c;
        }
        if (_size < S) {
            _str[_size] = 0;
        }
    }
    void append(const char* a, uint8_t length = 0xFF) {
        for (uint16_t i = 0;
                i < length && a[i] != 0 && _size < S;
                i++)
        {
            _str[_size++] = a[i];
        }
        if (_size < S) {
            _str[_size] = 0;
        }
    }
    template <uint8_t OS>
    void append(const SString<OS>& other) {
        append(other.c_str(), other.size());
    }

    void append(uint16_t num, SStringNumFormat format = SString_DEC,
            uint8_t minLength = 1, char padding = '0')
    {
        char tmp[16];
        uint8_t p = 0;
        do {
            tmp[p] = num % format;
            num = num / format;
            p++;
        } while (num > 0);
        while (minLength > p) {
            append(padding);
            minLength--;
        }
        do {
            p--;
            char c = tmp[p];
            c += '0';
            if (c > '9') {
                c += 7;
            }
            append(c);
        } while (p > 0);
    }

    template <uint8_t OS>
    void operator=(const SString<OS>& other) {
        _size = 0;
        append(other);
    }
    void operator=(const char* a) {
        _size = 0;
        append(a);
    }

    template <uint8_t OS>
    bool operator==(const SString<OS>& other) const {
        if (_size == other.size()) {
            return operator==(other.c_str());
        }
        return false;
    }
    bool operator==(const char* other) const {
        for (uint16_t i = 0; i < _size; i++) {
            if (other[i] == 0 || _str[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    void operator+=(char c) {
        append(c);
    }
    void operator+=(const char* a) {
        append(a);
    }
    template <uint8_t OS>
    void operator+=(const SString<OS>& other) {
        append(other);
    }

    const char* c_str() const {
        return _str;
    }

    size_t find(char c, uint8_t pos = 0) const {
        for (uint16_t i = pos; i < _size; i++) {
            if (_str[i] == c) {
                return i;
            }
        }
        return -1;
    }

    SString<S> substr(uint8_t pos = 0, uint8_t len = S) const {
        SString<S> ret;
        for (uint16_t i = pos; i < (pos + len) && i < _size; i++) {
            ret.append(_str[i]);
        }
        return ret;
    }

protected:
    char _str[S];
};

template<uint8_t S>
void serialize(ByteVector& buffer, const SString<S>& value) {
    serialize(buffer, value.c_str(), value.length());
}
template<uint8_t S>
void unserialize(ByteVector& buffer, SString<S>& value) {
    char c = 0;
    do {
        if (c != 0) {
            value.append(c);
        }
        unserialize(buffer, c);
    } while (c != 0 && value.size() < value.max_size());
}

template<class C>
C& writeToStream(C& o,const char* str, uint8_t len) {
    for (uint16_t i = 0; i < len; i++) {
        o << str[i];
    }
    return o;
}

/*
 * SString can be streamed in any class that has the operator << defined for
 * chars.
 */
template<class C, uint8_t S>
C& operator<<(C& o, const SString<S>& data) {
    return writeToStream(o, data.c_str(), data.size());
}

} /* namespace cometos_v6 */
#endif /* SSTRING_H_ */
