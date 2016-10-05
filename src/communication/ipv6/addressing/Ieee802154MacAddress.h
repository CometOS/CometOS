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
 * @author Andreas Weigel
 */

#ifndef IEEE802514MACADDRESS_H_
#define IEEE802514MACADDRESS_H_

namespace cometos_v6 {

#include <stdint.h>

class Ieee802154MacAddress {
public:
    Ieee802154MacAddress(const Ieee802154MacAddress& other);

    Ieee802154MacAddress(uint16_t shortPart = 0);

    Ieee802154MacAddress(const uint16_t* a);


    Ieee802154MacAddress(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4);

    // TODO buggy
//    Ieee802154MacAddress(uint64_t address) {
//        addr[0] = address >> 48 & 0xFF;
//        addr[1] = address >> 32 & 0xFF;
//        addr[2] = address >> 16 & 0xFF;
//        addr[3] = address & 0xFF;
//    }

    uint16_t a1() const {
        return addr[0];
    }
    uint16_t a2() const {
        return addr[1];
    }
    uint16_t a3() const {
        return addr[2];
    }
    uint16_t a4() const {
        return addr[3];
    }

    void setA1(uint16_t a) {
        addr[0] = a;
    }
    void setA2(uint16_t a) {
        addr[1] = a;
    }
    void setA3(uint16_t a) {
        addr[2] = a;
    }
    void setA4(uint16_t a) {
        addr[3] = a;
    }

    Ieee802154MacAddress& operator=(Ieee802154MacAddress const & other) {
        addr[0] = other.a1();
        addr[1] = other.a2();
        addr[2] = other.a3();
        addr[3] = other.a4();
        return *this;
    }

private:
    uint16_t addr[4];
};


bool operator==(Ieee802154MacAddress const & lhs, Ieee802154MacAddress const & rhs);
bool operator!=(Ieee802154MacAddress const & lhs, Ieee802154MacAddress const & rhs);

} // namespace cometos_v6

#endif /* IEEE802514MACADDRESS_H_ */
