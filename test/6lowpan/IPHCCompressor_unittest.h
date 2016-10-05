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

#ifndef IPHCCompressor_UNITTEST_H_
#define IPHCCompressor_UNITTEST_H_
#include "gtest/gtest.h"
#include "IPHCCompressor.h"
#include "LowpanBuffer.h"
#include "UDPPacket.h"
#include "OutputStream.h"

namespace {
class IPHCCompressorTest : public ::testing::Test {
    public:
        void SetUp() {
            datagram.addHeader(&udppacket);
        }
        void TearDown() {
            while(datagram.getNextHeader() != NULL){
                datagram.removeHeader(0, false);
            }
        }

        IPHCCompressorTest() :
            buf(200,2)
        {}

    protected:
//        cometos_v6::LowpanBuffer<200,2>       buf;
        cometos_v6::DynLowpanBuffer buf;
        cometos_v6::IPv6Datagram  datagram;
        cometos_v6::UDPPacket udppacket;
        cometos_v6::IPv6HopByHopOptionsHeader hopbyhop;

};

TEST_F(IPHCCompressorTest, compressCompleteUDP) {

    const char* srcAddresses[2] = {"1::1", "FE80::25"};
    const char* dstAddresses[2] = {"1::2", "FE80::117"};
    int expectedSizes[2][2] = { 65, 57, 57, 49 };


    uint8_t numSrcAddresses = sizeof(srcAddresses)/sizeof(char*);
    uint8_t numDstAddresses = sizeof(dstAddresses)/sizeof(char*);

    LOG_DEBUG("sizeof(srcAddresses)=" << sizeof(srcAddresses)
              << "|sizeof(char*)=" << sizeof(char*)
              << "|sizeof(IPv6Address)=" << sizeof(IPv6Address));

    LOG_DEBUG("srcPort=" << udppacket.getSrcPort() << "|dstPort=" << udppacket.getDestPort());

    uint8_t udpSrcPortH = (uint8_t)(udppacket.getSrcPort() >> 8);
    uint8_t udpSrcPortL = (uint8_t) udppacket.getSrcPort();
    uint8_t udpDstPortH = (uint8_t)(udppacket.getDestPort() >> 8);
    uint8_t udpDstPortL = (uint8_t)udppacket.getDestPort();

    uint32_t fl = 0x12345;
    uint8_t tc = 0x12;

    uint8_t checkContent[2][2][43]= {
            {
                {
                 // both ipv6 addr are global, no compression possible; same goes for
                 // TC and FL. HL is elided, as is NH field
                 0x65, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl),
                 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                 0xF0, udpSrcPortH, udpSrcPortL, udpDstPortH, udpDstPortL
                },
                {
                 // dst ipv6 addr compressible to 8byte; TC, FL inline; HL/NH elided
                 0x65, 0x01, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl),
                 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x17,
                 0xF0, udpSrcPortH, udpSrcPortL, udpDstPortH, udpDstPortL,
                 0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                }
            },
            {
                {    // dst ipv6 addr compressible to 8byte; TC, FL inline; HL/NH elided
                 0x65, 0x10, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl),
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25,
                 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                 0xF0, udpSrcPortH, udpSrcPortL, udpDstPortH, udpDstPortL,
                 0x01, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                },
                {
                 0x65, 0x11, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl),
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x17,
                 0xF0, udpSrcPortH, udpSrcPortL, udpDstPortH, udpDstPortL,
                 0x01, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                }
            }
        };

    for (uint8_t i = 0; i < numSrcAddresses; i++) {
        for (uint8_t j = 0; j < numDstAddresses; j++) {
            LOG_DEBUG("i=" << (int) i << "|j=" << (int) j);
            cometos_v6::BufferInformation* bi = buf.getBuffer(20);
            bi->clean();
            cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
            cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);
            datagram.src.tryParse(srcAddresses[i]);
            datagram.dst.tryParse(dstAddresses[j]);

            datagram.setFlowLabel(fl);
            datagram.setTrafficClass(tc);
            datagram.setHopLimit(1);

            udppacket.setData(bi->getContent(), bi->getSize());
            udppacket.generateChecksum(datagram.src, datagram.dst);

            cometos_v6::IPHCCompressor  cd(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
            cometos::Airframe frame;

            uint8_t maxSize = 80;
            uint16_t posInBuffer = 0;
            uint16_t size = cd.streamDatagramToFrame(frame, maxSize, bi, posInBuffer);
            EXPECT_EQ(datagram.getUpperLayerPayloadLength() + datagram.getCompleteHeaderLength(), size);
            EXPECT_EQ(expectedSizes[i][j], frame.getLength());

//            frame.printFrame(&cometos::getCout());

            for (uint8_t bi = 0; bi < 43; bi++) {
                LOG_DEBUG("bi=" << (int) bi);
                uint8_t b;
                frame >> b;
                EXPECT_EQ((uint8_t)checkContent[i][j][bi], b);
            }

            bi->free();
        }
    }
}

TEST_F(IPHCCompressorTest, compressBorderCases) {
    cometos_v6::BufferInformation* bi = buf.getBuffer(60);
    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    datagram.src.tryParse("1::1");
    datagram.dst.tryParse("1::2");

    uint32_t fl = 0x12345;
    uint8_t tc = 0x12;

    datagram.setFlowLabel(fl);
    datagram.setTrafficClass(tc);
    datagram.setHopLimit(1);

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    cometos_v6::IPHCCompressor  cd(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
    cometos_v6::IPHCCompressor  cd2(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
    cometos::Airframe frame;

    uint8_t maxSize = 80;
    uint16_t posInBuffer = 0;
    uint16_t size = cd.streamDatagramToFrame(frame, maxSize, bi, posInBuffer);

    // TODO re-evaluate requirements for compression
    EXPECT_EQ(72, size);
    EXPECT_EQ(71, frame.getLength());

    uint8_t checkContent[43] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::UDPPacket::HeaderNumber,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
            (uint8_t)(udppacket.getSrcPort() >> 8), (uint8_t)udppacket.getSrcPort(),
            (uint8_t)(udppacket.getDestPort() >> 8), (uint8_t)udppacket.getDestPort()};

    for (uint8_t i = 0; i < 43; i++) {
        uint8_t b;
        frame >> b;
        EXPECT_EQ(checkContent[i], b);
    }

    bi->free();
}

TEST_F(IPHCCompressorTest, testWithExtensionHeaders) {
    uint8_t bufq[118];
    for(uint8_t i = 0; i < 118; i++){
        bufq[i] = 2*i;
    }
    datagram.addHeader(&hopbyhop);
    cometos_v6::BufferInformation* bi = buf.getBuffer(60);
    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    datagram.src.tryParse("1::1");
    datagram.dst.tryParse("1::2");
    uint8_t buffer[60];
    for(uint8_t i = 0; i < 60; i++){
        buffer[i] = i;
    }
    bi->copyToBuffer(buffer, 60);
    uint32_t fl = 0x12345;
    uint8_t tc = 0x12;

    datagram.setFlowLabel(fl);
    datagram.setTrafficClass(tc);
    datagram.setHopLimit(1);

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    hopbyhop.setData(bufq,118);
    cometos_v6::IPHCCompressor  cd(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
    cometos::Airframe frame;
    cometos::Airframe frame2;
    cometos::Airframe frame3;
    uint8_t maxSize = 81;
    uint16_t posInBuffer = 0;
    uint16_t size = cd.streamDatagramToFrame(frame, maxSize, bi, posInBuffer);
    EXPECT_EQ(72, size);
    {/*uint8_t checkContent[43] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::UDPPacket::HeaderNumber,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
            (uint8_t)(udppacket.getSrcPort() >> 8), (uint8_t)udppacket.getSrcPort(),
            (uint8_t)(udppacket.getDestPort() >> 8), (uint8_t)udppacket.getDestPort()};
     */
        uint8_t checkContent[71] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::IPv6HopByHopOptionsHeader::HeaderNumber,
                0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
                17, 14, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22,
                24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48,
                50, 52, 54, 56, 58};

        for (uint8_t i = 0; i < 71; i++) {
            uint8_t b;
            frame >> b;
            EXPECT_EQ(checkContent[i], b);
        }
    }
    maxSize = 81;
    size = cd.streamDatagramToFrame(frame2, maxSize, bi, posInBuffer);
    EXPECT_EQ(144, size);
    {
        uint8_t checkContent[72];
        for (uint8_t i =0; i < 72; i++){
            checkContent[i] = 60 + 2*i;
        }
        for (uint8_t i = 0; i < 72; i++) {
            uint8_t b;
            frame2 >> b;
//            std::cout << "i=" << (int) i << std::endl;
            EXPECT_EQ(checkContent[i], b);
        }
    }

    maxSize = 81;
    size = cd.streamDatagramToFrame(frame3, maxSize, bi, posInBuffer);
    EXPECT_EQ(216, size);

    {
        {
            uint8_t checkContent[24];
            for (uint8_t i =0; i < 16; i++){
                checkContent[i] = 204 + 2*i;
            }
            checkContent[16] = (uint8_t)(udppacket.getSrcPort() >> 8);
            checkContent[17] = (uint8_t)udppacket.getSrcPort();
            checkContent[18] = (uint8_t)(udppacket.getDestPort() >> 8);
            checkContent[19] = (uint8_t)udppacket.getDestPort();
            for (uint8_t i = 0; i < 20; i++) {
                uint8_t b;
                frame3 >> b;
                EXPECT_EQ(checkContent[i], b);
            }
        }
    }

    bi->free();
}

}



#endif /* IPHCCompressor_UNITTEST_H_ */
