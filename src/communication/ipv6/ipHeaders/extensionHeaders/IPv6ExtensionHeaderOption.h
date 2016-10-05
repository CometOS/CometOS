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
 * @author: Martin Ringwelski
 */


#ifndef IPV6EXTENSIONHEADEROPTION_H_
#define IPV6EXTENSIONHEADEROPTION_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

class IPv6ExtensionHeaderOption {
public:
    IPv6ExtensionHeaderOption ():
        type(0),
        length(0),
        data(NULL)
    {}
    IPv6ExtensionHeaderOption (uint8_t type):
        type(type),
        length(0),
        data(NULL)
    {}
    uint8_t getType () const {
        return type;
    }
    uint8_t getLength () const {
        return length;
    }
    uint8_t* getOption () {
        return data;
    }
    void setType (uint8_t type) {
        this->type = type;
    }
    void setData (uint8_t* data,
                  uint8_t length) {
        this->data = data;
        this->length = length;
    }
protected:
    uint8_t type;
    uint8_t length;
    uint8_t* data;
};

}

#endif /* IPV6EXTENSIONHEADEROPTION_H_ */
