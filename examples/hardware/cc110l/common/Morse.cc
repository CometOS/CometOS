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

#include "Morse.h"
#include "TaskScheduler.h"
#include <string.h>
#include "OutputStream.h"

using namespace cometos;

static const char* codes[] = {
" / ",      // SPACE
"-.-.--",   // !
".-..-.",   // "
"?",        // #
"...-..-",  // $
"?",        // %
".-...",    // &
".----.",   // '
"-.--.",    // (
"-.--.-",   // )
"?",        // *
".-.-.",    // +
"--..--",   // ,
"-....-",   // -
".-.-.-",   // .
"-..-.",    // /
"-----",    // 0
".----",    // 1
"..---",    // 2
"...--",    // 3
"....-",    // 4
".....",    // 5
"-....",    // 6
"--...",    // 7
"---..",    // 8
"----.",    // 9
"---...",   // :
"-.-.-.",   // ;
"?",        // <
"-...-",    // =
"?",        // >
"..--..",   // ?
".--.-.",   // @
".-",       // A
"-...",     // B
"-.-.",     // C
"-..",      // D
".",        // E
"..-.",     // F
"--.",      // G
"....",     // H
"..",       // I
".---",     // J
"-.-",      // K
".-..",     // L
"--",       // M
"-.",       // N
"---",      // O
".--.",     // P
"--.-",     // Q
".-.",      // R
"...",      // S
"-",        // T
"..-",      // U
"...-",     // V
".--",      // W
"-..-",     // X
"-.--",     // Y
"--..",     // Z
"?",        // [
"?",        // backslash
"?",        // ]
"?",        // ^
"..--.-",   // _
"?"         // `
};

Morse::Morse() {
}

void Morse::appendSymbol(char symbol) {
    uint8_t onDits = 0;
    uint8_t offDits = 0;

    getCout() << (unsigned char)symbol;

    switch(symbol) {
    case '.':
        // on 1 dit, off 1 dit
        onDits = 1;
        offDits = 1;
        break;
    case '-':
        // on 3 dits, off 1 dit
        onDits = 3;
        offDits = 1;
        break;
    case ' ':
        // off 2 dits (1 dit already as end of previous symbol)
        onDits = 0;
        offDits = 2;
        break;
    case '/':
        // off 2 dits (1 dit already as end of previous symbol, 2*2 dits for enclosing spaces)
        onDits = 0;
        offDits = 2;
        break;
    default:
        ASSERT(false);
    }

    for(uint8_t i = 0; i < onDits; i++) {
        ASSERT(dit/8 < sizeof(buffer));
        buffer[dit/8] = (buffer[dit/8] << 1) | 1;
        dit++;
    }

    for(uint8_t i = 0; i < offDits; i++) {
        ASSERT(dit/8 < sizeof(buffer));
        buffer[dit/8] = (buffer[dit/8] << 1);
        dit++;
    }
}

void Morse::initMorse(const char* code, uint8_t length) {
    uint8_t position = 0;
    dit = 0;

    do {
        if(position >= length) {
            appendSymbol('/');
            appendSymbol(' ');
        }
        else {
            char c = code[position];
            if(c >= 97 && c <= 122) {
                // lower case character
                c -= ('a'-'A');
            }

            if(c >= 32 && c <= 96) {
                uint8_t idx = c-32;
                getCout() << (unsigned char)c << " ";
                for(uint8_t i = 0; i < strlen(codes[idx]); i++) {
                    appendSymbol(codes[idx][i]);
                }
                appendSymbol(' ');
                getCout() << endl;
            }
            else {
                ASSERT(false);
            }
        }

        position++;
    } while(position <= length);

    totalDits = dit;
    dit = 0;
}

bool Morse::nextDit() {
    if(dit >= totalDits) {
        return false;
    }

    bool on = (buffer[dit/8] >> (7-dit%8)) & 1;
    dit++;
    return on;
}

uint16_t Morse::ditsLeft() {
    return totalDits-dit;
}

void Morse::restart() {
    dit = 0;
}
