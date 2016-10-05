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

#ifndef IPV6DATAGRAM_UNITTEST_H_
#define IPV6DATAGRAM_UNITTEST_H_

#include "gtest/gtest.h"
#include "IPv6Datagram.h"

TEST(IPv6DatagramTest, setting) {
    cometos_v6::IPv6Datagram datagram;

    datagram.setFlowLabel(0x12345);
    datagram.setHopLimit(64);
    datagram.setTrafficClass(0x12);

    EXPECT_TRUE(0x12345 == datagram.getFlowLabel());
    EXPECT_EQ(0x12, datagram.getTrafficClass());

    for (int8_t i = 64; i > -1; i--) {
        EXPECT_EQ(i, datagram.getHopLimit());
        datagram.decrHopLimit();
    }
    EXPECT_EQ(0, datagram.getHopLimit());
}

TEST(IPv6DatagramTest, dumping) {
    cometos_v6::IPv6Datagram datagram;

    datagram.setFlowLabel(0x12345);
    datagram.setHopLimit(64);
    datagram.setTrafficClass(0x12);

    datagram.src.set(1,2,3,4,5,6,7,8);
    datagram.dst.set(11,12,13,14,15,16,17,18);

    uint8_t buffer[40];
    uint8_t expected[40] = {
            0x61, 0x21, 0x23, 0x45, 0x00, 0x00, cometos_v6::FollowingHeader::NoNextHeaderNumber, 64,
            0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
            0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18
    };

    ASSERT_EQ(40, datagram.writeHeaderToBuffer(buffer));

    for (uint8_t i = 0; i < 40; i++) {
        EXPECT_EQ(expected[i], buffer[i]);
    }
}

TEST(IPv6DatagramTest, parsing) {
    cometos_v6::IPv6Datagram datagram;
    uint8_t input[40] = {
            0x61, 0x21, 0x23, 0x45, 0x00, 0x00, cometos_v6::FollowingHeader::NoNextHeaderNumber, 64,
            0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
            0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18
    };

    EXPECT_EQ(40, datagram.parse(input, 40));
    EXPECT_EQ(NULL, datagram.getNextHeader());
    EXPECT_EQ(0, datagram.getNumNextHeaders());


    EXPECT_TRUE(0x12345 == datagram.getFlowLabel());
    EXPECT_EQ(0x12, datagram.getTrafficClass());
    EXPECT_EQ(64, datagram.getHopLimit());

    EXPECT_EQ(IPv6Address(1,2,3,4,5,6,7,8), datagram.src);
    EXPECT_EQ(IPv6Address(11,12,13,14,15,16,17,18), datagram.dst);
}

TEST(IPv6DatagramTest, headerChain) {
    const uint8_t TC = 0x79;
    const uint32_t FL = 0x54211;
    const uint16_t ipv6PayloadLen = 416;
    const uint16_t totalSize = ipv6PayloadLen+cometos_v6::IPv6Datagram::IPV6_HEADER_SIZE;
    const uint8_t HL = 5;
    const uint16_t fragOffset = 3400 >> 3;
    const uint16_t dstPort = 0xbeef;
    const uint16_t srcPort = 0xdead;
    cometos_v6::IPv6Datagram datagram;
    uint8_t input[totalSize] = {
            0x67, 0x95, 0x42, 0x11, 0x01, 0xA0, cometos_v6::IPv6FragmentHeader::HeaderNumber, HL,
            0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8,
            0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18,
            cometos_v6::UDPPacket::HeaderNumber, 0, 0x0D, 0x49, 0x00, 0x00, 0x00, 0x0AA,
            0xde, 0xad, 0xbe, 0xef, 0x01, 0x98, 0x00, 0x00
    };

    EXPECT_EQ(totalSize, datagram.parse(input, totalSize));
    EXPECT_EQ(2, datagram.getNumNextHeaders());

    cometos_v6::IPv6FragmentHeader* fh = dynamic_cast<cometos_v6::IPv6FragmentHeader*>(datagram.getNextHeader(0));
    ASSERT_TRUE(fh != NULL);
    cometos_v6::UDPPacket* udp = dynamic_cast<cometos_v6::UDPPacket*>(datagram.getNextHeader(1));
    ASSERT_TRUE(udp != NULL);
    EXPECT_EQ(cometos_v6::IPv6FragmentHeader::HeaderNumber, fh->getHeaderType());

    EXPECT_EQ(fragOffset, fh->getFragmentOffset());
    EXPECT_EQ(true, fh->getMFlag());
    EXPECT_EQ((uint32_t) 0xAA, fh->getIdentification());

    EXPECT_EQ(dstPort, udp->getDestPort());
    EXPECT_EQ(srcPort, udp->getSrcPort());
    EXPECT_EQ(ipv6PayloadLen - cometos_v6::UDPPacket::UDP_HEADER_SIZE,
               udp->getUpperLayerPayloadLength() + cometos_v6::UDPPacket::UDP_HEADER_SIZE);

    EXPECT_TRUE(FL == datagram.getFlowLabel());
    EXPECT_EQ(TC, datagram.getTrafficClass());
    EXPECT_EQ(HL, datagram.getHopLimit());

    EXPECT_EQ(IPv6Address(1,2,3,4,5,6,7,8), datagram.src);
    EXPECT_EQ(IPv6Address(11,12,13,14,15,16,17,18), datagram.dst);
}

#endif /* IPV6DATAGRAM_UNITTEST_H_ */
