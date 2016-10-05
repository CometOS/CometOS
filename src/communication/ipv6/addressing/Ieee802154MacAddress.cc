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

#include "Ieee802154MacAddress.h"

namespace cometos_v6 {

Ieee802154MacAddress::Ieee802154MacAddress(const Ieee802154MacAddress& other):
        addr{other.a1(), other.a2(), other.a3(), other.a4()}
{}

Ieee802154MacAddress::Ieee802154MacAddress(uint16_t shortPart) :
        addr{0, 0x00ff, 0xfe00, shortPart}
{}

Ieee802154MacAddress::Ieee802154MacAddress(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4) :
        addr{a1, a2, a3, a4}
{}

Ieee802154MacAddress::Ieee802154MacAddress(const uint16_t* a) :
    addr{a[0], a[1], a[2], a[3]}
{
}

bool operator==(Ieee802154MacAddress const & lhs, Ieee802154MacAddress const & rhs) {
    return lhs.a1() == rhs.a1()
           && lhs.a2() == rhs.a2()
           && lhs.a3() == rhs.a3()
           && lhs.a4() == rhs.a4();
}

bool operator!=(Ieee802154MacAddress const & lhs, Ieee802154MacAddress const & rhs) {
    return !(lhs == rhs);
}

} // namespace cometos_v6
