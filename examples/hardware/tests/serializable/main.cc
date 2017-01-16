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

/**@file Testing Serializable class on MCU
 * @author Stefan Unterschuetz
 */
#include "cometos.h"
#include "logging.h"
#include "Module.h"
#include "Airframe.h"

#include "Serializable.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

class MyClass: public Serializable {
public:
    MyClass() :
            a(this), b(this), c(this) {
    }

    Field<uint8_t> a;
    Field<uint16_t> b;
    Field<uint8_t> c;
};

MyClass myClass2;


int main() {

    cometos::initialize();

    MyClass myClass;
    myClass.a=3;
    myClass.b=20000;
    myClass.c=255;
    AirframePtr air=new Airframe();
    (*air)<<myClass;


    (*air)>>myClass2;

    LOG_ERROR("a "<<(uint8_t)myClass2.a<<" b"<<(uint16_t)myClass2.b<<" c"<<(uint8_t)myClass2.c);

    cometos::run();
    return 0;
}
