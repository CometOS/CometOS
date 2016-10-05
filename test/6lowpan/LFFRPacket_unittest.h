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

#include "gtest/gtest.h"
#include <LFFRPacket.h>
#include "RetransmissionList.h"
#include "LFFRPacket.h"
#include "IPHCCompressor.h"
#include "IPv6Request.h"

namespace cometos_v6 {

class LFFRPacketTest : public testing::Test {
   public:
    void SetUp() {
        datagram.addHeader(&udppacket);
        ipInformation.req.datagram = &datagram;
    }
    void TearDown() {
        while(datagram.getNextHeader() != NULL){
            datagram.removeHeader(0, false);
        }
    }
    void testGetSeqNum();
    void testPopulateDataFromBufferSmallDatagram();
    void testPopulateDataFromBufferSequenceNumberOutofBounds();
    void testPopulateDataFromBufferLargeDatagram();
    void testClearSeqNumFromList();
    void testAddLFFRHeaders();
    void testWithExtensionHeaders();

   private:
    cometos_v6::LowpanBuffer<200, 2> buf;
    cometos_v6::IPv6Datagram datagram;
    cometos_v6::UDPPacket udppacket;
    cometos_v6::IPv6Request ipInformation;
    cometos_v6::DatagramInformation datagramInfo;
    cometos_v6::IPv6HopByHopOptionsHeader hopbyhopExtensionHeader;
};

TEST_F(LFFRPacketTest, testGetSeqNum) { testGetSeqNum(); }

TEST_F(LFFRPacketTest, testPopulateDataFromBufferSmallDatagram) {
    testPopulateDataFromBufferSmallDatagram();
}

TEST_F(LFFRPacketTest, testPopulateDataFromBufferSequenceNumberOutofBounds) {
    testPopulateDataFromBufferSequenceNumberOutofBounds();
}

TEST_F(LFFRPacketTest, testPopulateDataFromBufferLargeDatagram) {
    testPopulateDataFromBufferLargeDatagram();
}

TEST_F(LFFRPacketTest, testClearSeqNumFromList) { testClearSeqNumFromList(); }

TEST_F(LFFRPacketTest, testAddLFFRHeaders) { testAddLFFRHeaders(); }

TEST_F(LFFRPacketTest, testWithExtensionHeaders) {
    testWithExtensionHeaders();
}

void LFFRPacketTest::testGetSeqNum() {
    cometos_v6::DatagramInformation dummyInformation;
    uint32_t retransmissionList = 0x8FFFFFFF;

    cometos_v6::LFFRPacket testPkt1(&dummyInformation, retransmissionList);
    cometos_v6::LFFRPacket testPkt2(&dummyInformation, 0x0FFFFFFF);
    cometos_v6::LFFRPacket testPkt3(&dummyInformation, 0x00000001);
    cometos_v6::LFFRPacket testPkt4(&dummyInformation, 0x00000000);

    EXPECT_EQ(0, testPkt1.getNextSeqNumForTransmission());
    EXPECT_EQ(4, testPkt2.getNextSeqNumForTransmission());
    EXPECT_EQ(31, testPkt3.getNextSeqNumForTransmission());
    EXPECT_EQ(0xFF, testPkt4.getNextSeqNumForTransmission());
}

void LFFRPacketTest::testPopulateDataFromBufferSequenceNumberOutofBounds() {
    cometos_v6::BufferInformation* bi = buf.getBuffer(20);
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
    ipInformation.req.srcMacAddress = srcMAC;
    ipInformation.req.dstMacAddress = dstMAC;
    uint16_t datagramTag = 0;

    datagramInfo.setData(&ipInformation, bi, datagramTag);
    datagramInfo.setCompressedHeaderSize(40);
    datagramInfo.setSizeOfCompressedDatagram(60);
    LFFRPacket lffQueueObject(&datagramInfo);

    cometos::Airframe frame;
    uint8_t sequenceNumber = 1;
    bool isLastFrame = false;
    bool isFragmented = false;
    uint16_t offset = lffQueueObject.populateDataGramIntoFrame(
        frame, sequenceNumber, 72, isLastFrame, isFragmented);

    EXPECT_EQ(0, frame.getLength());
    EXPECT_TRUE(isLastFrame);
}

void LFFRPacketTest::testPopulateDataFromBufferSmallDatagram() {
    // i need a good datagram
    cometos_v6::BufferInformation* bi = buf.getBuffer(20);
    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);
    datagram.src.tryParse("1::1");
    datagram.dst.tryParse("1::2");
    uint32_t fl = 0x12345;
    uint8_t tc = 0x12;
    datagram.setFlowLabel(fl);
    datagram.setTrafficClass(tc);
    datagram.setHopLimit(1);

    uint8_t buff[20];
    for (uint8_t i = 0; i < 20; i++) {
        buff[i] = i;
    }
    bi->copyToBuffer(buff, 20);

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);
    ipInformation.req.srcMacAddress = srcMAC;
    ipInformation.req.dstMacAddress = dstMAC;
    uint16_t datagramTag = 0;

    datagramInfo.setData(&ipInformation, bi, datagramTag);
    LFFRPacket lffQueueObject(&datagramInfo);

    cometos::Airframe frame;
    uint8_t sequenceNumber = 0;
    bool isLastFrame = false;
    bool isFragmented = false;
    uint16_t offset = lffQueueObject.populateDataGramIntoFrame(
        frame, sequenceNumber, 81, isLastFrame, isFragmented);

    // check compression
    uint8_t checkContent[43] = {0x65,
                                0x00,
                                tc,
                                (uint8_t)(fl >> 16),
                                (uint8_t)(fl >> 8),
                                (uint8_t)(fl),
                                0,
                                1,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                1,
                                0,
                                1,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                2,
                                0xF0,
                                (uint8_t)(udppacket.getSrcPort() >> 8),
                                (uint8_t)udppacket.getSrcPort(),
                                (uint8_t)(udppacket.getDestPort() >> 8),
                                (uint8_t)udppacket.getDestPort()};

    EXPECT_EQ(65, frame.getLength());
    EXPECT_EQ(0, offset);
    EXPECT_TRUE(isLastFrame);

    for (uint8_t i = 0; i < 43; i++) {
        uint8_t b;
        frame >> b;
        EXPECT_EQ(checkContent[i], b);
    }

    bi->free();
}

void LFFRPacketTest::testPopulateDataFromBufferLargeDatagram() {

    cometos_v6::BufferInformation* bi = buf.getBuffer(110);
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

    uint16_t datagramTag = 15;
    ipInformation.req.srcMacAddress = srcMAC;
    ipInformation.req.dstMacAddress = dstMAC;
    datagramInfo.setData(&ipInformation, bi, datagramTag);
    LFFRPacket lffQueueObject(&datagramInfo);
    cometos_v6::IPHCCompressor cd(&datagram, srcMAC, dstMAC);

    uint8_t data[110];
    for (uint8_t i = 0; i < 110; i++) {
        data[i] = i;
    }
    bi->copyToBuffer(data, 110);

    cometos::Airframe frame;
    uint8_t maxSize = (81 - 6) & 0xF8;
    uint8_t sequenceNumber = 0;
    bool isLastFrame = false;
    bool isFragmented = false;
    uint16_t offset = lffQueueObject.populateDataGramIntoFrame(
        frame, sequenceNumber, 81, isLastFrame, isFragmented);

    uint8_t checkContent[43] = {0x61,
                                0x00,
                                tc,
                                (uint8_t)(fl >> 16),
                                (uint8_t)(fl >> 8),
                                (uint8_t)(fl),
                                cometos_v6::UDPPacket::HeaderNumber,
                                0,
                                1,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                1,
                                0,
                                1,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                2,
                                (uint8_t)(udppacket.getSrcPort() >> 8),
                                (uint8_t)udppacket.getSrcPort(),
                                (uint8_t)(udppacket.getDestPort() >> 8),
                                (uint8_t)udppacket.getDestPort()};

    EXPECT_EQ(72, frame.getLength());
    EXPECT_EQ(0, offset);
    EXPECT_FALSE(isLastFrame);
    uint8_t b;
    uint8_t count;
    for (uint8_t i = 0; i < 43; i++) {
        frame >> b;
        EXPECT_EQ(checkContent[i], b);
    }
    // remove UDP stuff
    frame >> b;
    frame >> b;
    frame >> b;
    frame >> b;

    // check data
    count = 0;
    while (frame.getLength() > 0) {
        frame >> b;
        EXPECT_EQ(data[count], b);
        count++;
    }

    cometos::Airframe frame2;
    sequenceNumber = 1;
    isLastFrame = false;
    isFragmented = false;
    offset = lffQueueObject.populateDataGramIntoFrame(frame2, sequenceNumber, 81,
                                                   isLastFrame, isFragmented);

    while (frame.getLength() > 0) {
        frame >> b;
        EXPECT_EQ(data[count], b);
        count++;
    }

    EXPECT_EQ(72, frame2.getLength());
    EXPECT_EQ(9, offset);
    EXPECT_FALSE(isLastFrame);

    cometos::Airframe frame3;
    sequenceNumber = 2;
    isLastFrame = false;
    isFragmented = false;
    offset = lffQueueObject.populateDataGramIntoFrame(frame3, sequenceNumber, 81,
                                                   isLastFrame, isFragmented);

    while (frame.getLength() > 0) {
        frame >> b;
        EXPECT_EQ(data[count], b);
        count++;
    }

    EXPECT_EQ(13, frame3.getLength());
    EXPECT_EQ(18, offset);
    EXPECT_TRUE(isLastFrame);

    bi->free();
}

void LFFRPacketTest::testClearSeqNumFromList() {
    LFFRPacket lffrQueueObject(&datagramInfo, 0x70000000);
    lffrQueueObject.clearSeqNumFrmTransmissionList(2);
    EXPECT_EQ(0x50000000, lffrQueueObject._transmissionList);
    lffrQueueObject.clearSeqNumFrmTransmissionList(31);
    EXPECT_EQ(0x50000000, lffrQueueObject._transmissionList);
}

void LFFRPacketTest::testAddLFFRHeaders() {
    LFFRPacket lffrQueueObject(&datagramInfo);
    cometos::Airframe frame1;
    cometos::Airframe frame2;
    uint16_t tag = 58;
    uint16_t size = 1280;
    uint8_t offset = 5;
    bool enableImplicitAck = true;
    uint8_t sequenceNumber = 8;

    lffrQueueObject.addLFFRHeader(frame1, size, sequenceNumber, tag, offset);
    EXPECT_EQ(6, frame1.getLength());
    uint8_t b;

    frame1 >> b;
    uint8_t RFRAG_INDICATION = 0xE8;
    EXPECT_EQ(RFRAG_INDICATION, b);

    frame1 >> b;
    EXPECT_EQ(offset, b);

    uint16_nbo receivedTag;
    frame1 >> receivedTag;
    EXPECT_EQ(tag, receivedTag.getUint16_t());

    uint8_t tmp;
    frame1 >> tmp;
    uint8_t receivedSequence = (tmp >> 3) & 0x8F;
    EXPECT_EQ(sequenceNumber, receivedSequence);

    uint8_t msbOfDgrmSize = tmp & 0x07;
    uint8_t lsbOfDgrmSize;
    frame1 >> lsbOfDgrmSize;
    uint16_t dGramSize = (((uint16_t)(msbOfDgrmSize) << 8) | (lsbOfDgrmSize));
    EXPECT_EQ(size, dGramSize);

    lffrQueueObject.addLFFRHeader(frame2, size, sequenceNumber, tag, offset);
    frame2 >> b;
    uint8_t RFRAG_ACK_INDICATION = 0xE9;
    EXPECT_EQ(RFRAG_INDICATION, b);
}

inline void LFFRPacketTest::testWithExtensionHeaders() {
    uint8_t bufq[118];
    for(uint8_t i = 0; i < 118; i++){
        bufq[i] = 2*i;
    }
    datagram.addHeader(&hopbyhopExtensionHeader);
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

    hopbyhopExtensionHeader.setData(bufq,118);
    cometos_v6::IPHCCompressor  cd(&datagram, srcMAC, dstMAC);
    cometos_v6::IPHCCompressor  cd2(&datagram, srcMAC, dstMAC);

    uint16_t datagramTag = 15;
    ipInformation.req.srcMacAddress = srcMAC;
    ipInformation.req.dstMacAddress = dstMAC;
    datagramInfo.setData(&ipInformation, bi, datagramTag);
    LFFRPacket lffQueueObject(&datagramInfo);


    cometos::Airframe frame;
    cometos::Airframe frame2;
    cometos::Airframe frame3;
    cometos::Airframe frame4;
    uint8_t maxSize = 81;
    uint16_t posInBuffer = 0;
    bool isLastFrame;
    bool isFragmented = false;
    uint8_t sequenceNumber = 0;
    uint8_t offset = lffQueueObject.populateDataGramIntoFrame(frame, sequenceNumber, maxSize, isLastFrame, isFragmented);
    EXPECT_EQ(72, frame.getLength());
    EXPECT_EQ(0, offset);
    EXPECT_EQ(true, isFragmented);
    EXPECT_EQ(false, isLastFrame);

    {/*uint8_t checkContent[43] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::UDPPacket::HeaderNumber,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
            (uint8_t)(udppacket.getSrcPort() >> 8), (uint8_t)udppacket.getSrcPort(),
            (uint8_t)(udppacket.getDestPort() >> 8), (uint8_t)udppacket.getDestPort()};
     */
        uint8_t checkContent[72] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::IPv6HopByHopOptionsHeader::HeaderNumber,
                0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
                17, 14, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22,
                24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48,
                50, 52, 54, 56, 58, 60};

        for (uint8_t i = 0; i < 72; i++) {
            uint8_t b;
            frame >> b;
            EXPECT_EQ(checkContent[i], b);
        }
    }
    maxSize = 81;
    sequenceNumber = 1;
    offset = lffQueueObject.populateDataGramIntoFrame(frame2, sequenceNumber, maxSize, isLastFrame, isFragmented);
    EXPECT_EQ(9, offset);
    EXPECT_EQ(72, frame2.getLength());
    EXPECT_FALSE(isLastFrame);
    EXPECT_TRUE(isFragmented);
    {
        uint8_t checkContent[72];
        for (uint8_t i =0; i < 72; i++){
            checkContent[i] = 62 + 2*i;
        }
        for (uint8_t i = 0; i < 72; i++) {
            uint8_t b;
            frame2 >> b;
            EXPECT_EQ(checkContent[i], b);
        }
    }

    maxSize = 81;
    sequenceNumber = 2;
    offset = lffQueueObject.populateDataGramIntoFrame(frame3, sequenceNumber, maxSize, isLastFrame, isFragmented);
    EXPECT_EQ(72, frame3.getLength());
    EXPECT_EQ(18, offset);
    EXPECT_EQ(false, isLastFrame);
    EXPECT_EQ(true, isFragmented);
    uint8_t count = 0;
    {
        {
            uint8_t checkContent[72];
            for (uint8_t i =0; i < 15; i++){
                checkContent[i] = 206 + 2*i;
            }
            checkContent[15] = (uint8_t)(udppacket.getSrcPort() >> 8);
            checkContent[16] = (uint8_t)udppacket.getSrcPort();
            checkContent[17] = (uint8_t)(udppacket.getDestPort() >> 8);
            checkContent[18] = (uint8_t)udppacket.getDestPort();
            for (uint8_t i = 0; i < 19; i++) {
                uint8_t b;
                frame3 >> b;
                EXPECT_EQ(checkContent[i], b);
            }
            for(uint8_t i = 23; i < 49; i++){
                checkContent[i] = count++;
            }
            for(uint8_t i = 23; i < 49; i++){
                uint8_t b;
                frame3>>b;
                checkContent[i] == b;
            }

        }
    }
    maxSize = 81;
    sequenceNumber = 3;
    offset = lffQueueObject.populateDataGramIntoFrame(frame4, sequenceNumber, maxSize, isLastFrame, isFragmented);
    EXPECT_EQ(11, frame4.getLength());
    EXPECT_EQ(27, offset);
    EXPECT_EQ(true, isLastFrame);
    EXPECT_EQ(true, isFragmented);
    {
        uint8_t checkContent[72];
        for(uint8_t i = 0; i < 11; i++){
            checkContent[i] = count++;
        }
            for(uint8_t i = 0; i < 11; i++){
                uint8_t b;
                frame4>>b;
                checkContent[i] == b;
            }
    }
    maxSize = 81;
    sequenceNumber = 4;
    cometos::Airframe frame5;
    offset = lffQueueObject.populateDataGramIntoFrame(frame5, sequenceNumber, maxSize, isLastFrame, isFragmented);
    EXPECT_EQ(0, frame5.getLength());
    bi->free();

}





} /* namespace cometos_v6 */
