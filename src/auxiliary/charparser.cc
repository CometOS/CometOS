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

#include "charparser.h"
#include "cometos.h"

bool getLine(const char* buf, char line[], uint8_t offset) {
    uint8_t cnt = 0;

    while(buf[offset] != '\n' && offset < strlen(buf)){
        line[cnt] = buf[offset];
        offset++;
        cnt++;
    }
    //set unused elements to zero
    for (size_t i=cnt; i<strlen(buf); i++) {
        line[i] = 0;
    }

    if(buf[offset] != '\n') {
        // end of array buf was reached
        return true;
    }
    // terminating char \n was found
    return false;
}


void trimChArray(const char* buf, char trimBuf[], const char* delimiters, uint8_t nDelimiters) {
    uint8_t i, j;
    uint8_t cnt = 0;
    uint8_t nBuf = strlen(buf);
    bool isDelim;

    for(i=0; i<nBuf; i++) {
        isDelim = false;
        for(j=0; j<nDelimiters; j++) {
            if((buf[i]==delimiters[j]) || (buf[i]==0 )) {
                isDelim = true;
            }
        }
        //store all non-zero elements, that are not delimiters in a tmp char[]
        if(!isDelim) {
            trimBuf[cnt] = buf[i];
            cnt++;
        }
    }
    //set unused elements to zero
    for (int k=cnt; k<nBuf; k++) {
        trimBuf[k] = 0;
    }
}





//Inner while-loop from Logger.cc; used before charparser existed
/**
// Retrieve one line at a time
while(rxBuf[i] != '\n'){
    // check for delimiter of config parameters
    if((rxBuf[i] == ',') && (tmpBuf != NULL)) {
        tokens.pushBack(tmpBuf);
        tmpBuf = NULL;
        i+=2;
        offset = i;
    } else if (strcmp(rxBuf[i],"//") == 0 || strcmp(rxBuf[i],"#") == 0) {
        // skip comments
        while(rxBuf[i] != '\n'){
            i++;
        }
        continue;
    } else {
        while((rxBuf[i] == '\t') || (rxBuf[i] == '\r') || (rxBuf[i] == ' ')) {
            i++;
        }
    }
    // this has to be here (after the if-statement), so that the "," isn't in the buffer
    tmpBuf[i - offset] = rxBuf[i];
    i++;
}**/
