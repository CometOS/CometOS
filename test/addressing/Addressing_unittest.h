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

#ifndef ADDRESSING_UNITTEST_H_
#define ADDRESSING_UNITTEST_H_

#include "gtest/gtest.h"
#include "IPv6Address.h"
#include "Ieee802154MacAddress.h"

TEST(AddressingTest, IPv6AddressSet) {
    IPv6Address a(1,2,3,4,5,6,7,8);
    IPv6Address b(0,1,0,2,0,3,0,4,0,5,0,6,0,7,0,8);
    IPv6Address c("1:2:3:4:5:6:7:8");
    for (uint8_t i = 0 ; i < 8; i++) {
        EXPECT_EQ(i+1, a.getAddressPart(i));
        EXPECT_EQ(i+1, b.getAddressPart(i));
        EXPECT_EQ(i+1, c.getAddressPart(i));
    }

    for (uint8_t i = 8; i > 0; i++) {
        EXPECT_EQ(0, a.getAddressPart(i));
    }

    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == c);
    EXPECT_TRUE(c == a);
}

TEST(AddressingTest, IPv6AddressTest) {
    IPv6Address a;
    IPv6Address b;
    IPv6Address c("FF00::");
    IPv6Address d("::1");
    IPv6Address e("FF02::1");

    a.setLinkLocal();

    EXPECT_TRUE(a > b);
    EXPECT_TRUE(b < a);

    EXPECT_FALSE(a.isUnspecified());
    EXPECT_TRUE(a.isLinkLocal());
    EXPECT_TRUE(a.isUnicast());
    EXPECT_FALSE(a.isLoopbackBroadcast());
    EXPECT_FALSE(a.isLoopbackRouterBroadcast());
    EXPECT_FALSE(a.isLocalBroadcast());
    EXPECT_FALSE(a.isLocalRouterBroadcast());
    EXPECT_FALSE(a.isLoopback());
    EXPECT_FALSE(a.isMulticast());
    EXPECT_FALSE(a.isSiteRouterBroadcast());

    EXPECT_TRUE(b.isUnspecified());
    EXPECT_FALSE(b.isLinkLocal());
    EXPECT_FALSE(b.isUnicast());
    EXPECT_FALSE(b.isLoopbackBroadcast());
    EXPECT_FALSE(b.isLoopbackRouterBroadcast());
    EXPECT_FALSE(b.isLocalBroadcast());
    EXPECT_FALSE(b.isLocalRouterBroadcast());
    EXPECT_FALSE(b.isLoopback());
    EXPECT_FALSE(b.isMulticast());
    EXPECT_FALSE(b.isSiteRouterBroadcast());

    EXPECT_FALSE(c.isUnspecified());
    EXPECT_FALSE(c.isLinkLocal());
    EXPECT_FALSE(c.isUnicast());
    EXPECT_FALSE(c.isLoopbackBroadcast());
    EXPECT_FALSE(c.isLoopbackRouterBroadcast());
    EXPECT_FALSE(c.isLocalBroadcast());
    EXPECT_FALSE(c.isLocalRouterBroadcast());
    EXPECT_FALSE(c.isLoopback());
    EXPECT_TRUE(c.isMulticast());
    EXPECT_FALSE(c.isSiteRouterBroadcast());

    EXPECT_FALSE(d.isUnspecified());
    EXPECT_FALSE(d.isLinkLocal());
    EXPECT_TRUE(d.isUnicast());
    EXPECT_FALSE(d.isLoopbackBroadcast());
    EXPECT_FALSE(d.isLoopbackRouterBroadcast());
    EXPECT_FALSE(d.isLocalBroadcast());
    EXPECT_FALSE(d.isLocalRouterBroadcast());
    EXPECT_TRUE(d.isLoopback());
    EXPECT_FALSE(d.isMulticast());
    EXPECT_FALSE(d.isSiteRouterBroadcast());

    EXPECT_FALSE(e.isUnspecified());
    EXPECT_FALSE(e.isLinkLocal());
    EXPECT_FALSE(e.isUnicast());
    EXPECT_FALSE(e.isLoopbackBroadcast());
    EXPECT_FALSE(e.isLoopbackRouterBroadcast());
    EXPECT_TRUE(e.isLocalBroadcast());
    EXPECT_FALSE(e.isLocalRouterBroadcast());
    EXPECT_FALSE(e.isLoopback());
    EXPECT_TRUE(e.isMulticast());
    EXPECT_FALSE(e.isSiteRouterBroadcast());

}

TEST(AddressingTest, IEEE802154Set) {
    cometos_v6::Ieee802154MacAddress a((uint16_t)1);
    EXPECT_EQ(0, a.a1());
    EXPECT_EQ(0x00ff, a.a2());
    EXPECT_EQ(0xfe00, a.a3());
    EXPECT_EQ(1, a.a4());

    cometos_v6::Ieee802154MacAddress b(a);
    EXPECT_EQ(a.a1(), b.a1());
    EXPECT_EQ(a.a2(), b.a2());
    EXPECT_EQ(a.a3(), b.a3());
    EXPECT_EQ(a.a4(), b.a4());

    uint16_t array[4] = {1, 2, 3, 4};
    cometos_v6::Ieee802154MacAddress c(array);
    EXPECT_EQ(1, c.a1());
    EXPECT_EQ(2, c.a2());
    EXPECT_EQ(3, c.a3());
    EXPECT_EQ(4, c.a4());

    cometos_v6::Ieee802154MacAddress d(1, 2, 3, 4);
    EXPECT_EQ(1, d.a1());
    EXPECT_EQ(2, d.a2());
    EXPECT_EQ(3, d.a3());
    EXPECT_EQ(4, d.a4());


    cometos_v6::Ieee802154MacAddress f;
    f = c;
    EXPECT_EQ(c.a1(), f.a1());
    EXPECT_EQ(c.a2(), f.a2());
    EXPECT_EQ(c.a3(), f.a3());
    EXPECT_EQ(c.a4(), f.a4());

    cometos_v6::Ieee802154MacAddress g;
    g.setA1(1);
    g.setA2(2);
    g.setA3(3);
    g.setA4(4);
    EXPECT_EQ(1, g.a1());
    EXPECT_EQ(2, g.a2());
    EXPECT_EQ(3, g.a3());
    EXPECT_EQ(4, g.a4());

}

#endif /* ADDRESSING_UNITTEST_H_ */
