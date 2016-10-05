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
 * @author: Martin Ringwelski
 */

#ifndef TOKEN_H_
#define TOKEN_H_

#include <cometos.h>

namespace cometos_v6 {

class Token {
public:
    static const uint8_t MAX_LENGTH = 8;

    Token():
        _token{0,0,0,0,0,0,0,0}
    {}
    Token(const Token& token):
        _token{token._token[0],
        token._token[1],
        token._token[2],
        token._token[3],
        token._token[4],
        token._token[5],
        token._token[6],
        token._token[7]}
    {}

    virtual ~Token() {}

    uint8_t getTokenLength() {
        for (uint8_t i = 0; i < MAX_LENGTH; i++) {
            if (_token[i] != 0) {
                return (MAX_LENGTH - i);
            }
        }
        return 0;
    }

    void readToken(const uint8_t* buffer, uint8_t length)  {
        const uint8_t start = MAX_LENGTH - length;

        for(uint8_t i = 0; i < length; i++) {
            _token[start + i] = buffer[i];
        }
    }

    uint8_t getTokenPart(uint8_t part) {
        ASSERT(part < MAX_LENGTH);
        return _token[part];
    }

    void setTokenPart(uint8_t part, uint8_t t) {
        ASSERT(part < MAX_LENGTH);
        _token[part] = t;
    }

    void generate(uint8_t length = MAX_LENGTH) {
        for (uint8_t i = 0; i < length; i++) {
            _token[MAX_LENGTH - i - 1] = intrand(0xFF);
        }
    }

    bool operator==(const Token& other) {
        return (_token[0] == other._token[0] &&
                _token[1] == other._token[1] &&
                _token[2] == other._token[2] &&
                _token[3] == other._token[3] &&
                _token[4] == other._token[4] &&
                _token[5] == other._token[5] &&
                _token[6] == other._token[6] &&
                _token[7] == other._token[7]);
    }

    Token& operator=(const Token& other) {
        for (uint8_t i = 0; i < MAX_LENGTH; i++) {
            _token[i] = other._token[i];
        }
        return *this;
    }

    bool isZero() {
        return (_token[0] == 0 &&
                _token[1] == 0 &&
                _token[2] == 0 &&
                _token[3] == 0 &&
                _token[4] == 0 &&
                _token[5] == 0 &&
                _token[6] == 0 &&
                _token[7] == 0);
    }

protected:
    uint8_t _token[MAX_LENGTH];
};

} /* namespace cometos_v6 */
#endif /* TOKEN_H_ */
