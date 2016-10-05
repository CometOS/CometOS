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

#ifndef ASSEMBLYBUFFER_UNITTEST_H_
#define ASSEMBLYBUFFER_UNITTEST_H_

#include "gtest/gtest.h"
#include "LowpanBuffer.h"
#include "AssemblyBuffer.h"
#include "lowpan-macros.h"
#include "IPv6Request.h"
#include "QueuePacket.h"
#include "omnetppDummyEnv.h"
#include "FragMetadata.h"
#include "IPv6Datagram.h"

typedef cometos_v6::AssemblyBuffer<3> AssemblyBuffer_t;

namespace {
class assemblyBufferTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        OmnetppDummyEnv::setup();
    }

    void SetUp() {
        OmnetppDummyEnv::setup();
        datagram.addHeader(&udppacket);
        datagram.setHopLimit(1);
        datagram.src.tryParse("1::1");
        datagram.dst.tryParse("1::2");
        datagram.setFlowLabel(0x12345);
        datagram.setTrafficClass(0x12);

        tag = 0x45;

        udppacket.setSrcPort(0xF012);
        udppacket.setDestPort(0xF034);
    }
    void TearDown() {
        datagram.removeHeader(0, false);
    }

    assemblyBufferTest() :
        buf(1000, 5)
    {}


protected:
    cometos_v6::IPv6Datagram  datagram;
    cometos_v6::UDPPacket udppacket;
//    cometos_v6::LowpanBuffer<1000,5>       buf;
    cometos_v6::DynLowpanBuffer  buf;
    uint16_t  tag;
    IPv6Context srcCtx[16];
    IPv6Context dstCtx[16];
    const uint8_t MAX_FRAGMENT_SIZE = 80;
    cometos_v6::LowpanFragMetadata fm;
    const cometos_v6::IPv6Datagram* dg;
};

TEST_F(assemblyBufferTest, addInOrder) {

    const uint16_t BUF_SIZE = 300;
    AssemblyBuffer_t assemblyBuffer(&buf);

    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    cometos_v6::BufferInformation* bi = buf.getBuffer(BUF_SIZE);

    for (uint16_t i = 0; i < BUF_SIZE; i++) {
        (*bi)[i] = i;
    }

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    cometos_v6::IPv6Request req(&datagram);


    cometos_v6::QueuePacket qp(&req,tag,bi,0);

    uint8_t maxsize = MAX_FRAGMENT_SIZE;
    cometos::Airframe frame;
    qp.createFrame(frame, maxsize, fm, dg);

    ASSERT_TRUE(fm.isFirstOfDatagram());

    uint8_t head;
    frame >> head;

    ASSERT_TRUE(cometos_v6::isFragmentationHeader1(head));
    uint16_nbo  fSize;
    uint16_nbo  fTag;
    uint8_t     tmp;

    frame >> tmp >> fTag;
    fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));

    frame >> head;
    cometos_v6::IPHCDecompressor* dcip =
            new cometos_v6::IPHCDecompressor(
                    head, frame, srcMAC, dstMAC, srcCtx, dstCtx);

    ASSERT_FALSE(NULL == dcip->getIPDatagram());

    cometos_v6::bufStatus_t stat = cometos_v6::BS_SUCCESS;

    cometos_v6::DatagramReassembly* fi = assemblyBuffer.addFirstFragment(
            srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), frame, dcip, stat);

    ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);
    ASSERT_FALSE(NULL == fi);
    ASSERT_FALSE(fi->isDone());

    bool done = false;
    while (!done) {
        uint8_t nMaxSize = MAX_FRAGMENT_SIZE;
        cometos::Airframe nFrame;
        qp.createFrame(nFrame, nMaxSize, fm, dg);

        if (bi->getSize() == 0) done = true;

        nFrame >> head;

        ASSERT_TRUE(cometos_v6::isFragmentationHeaderX(head));

        nFrame >> tmp >> fTag;
        fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));
        uint8_t     fOffset;
        nFrame >> fOffset;

        fi = assemblyBuffer.addFragment(
                srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), fOffset, nFrame, stat);
        ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);
        ASSERT_FALSE(NULL == fi);
        EXPECT_EQ(done, fi->isDone());
    }

    bi->free();

    cometos_v6::IPv6Datagram* retDatagram = fi->getDatagram();
    ASSERT_TRUE(retDatagram != NULL);
    bi = fi->decapsulateBuffer();
    fi->free();
    EXPECT_EQ(datagram.src, retDatagram->src);
    EXPECT_EQ(datagram.dst, retDatagram->dst);
    EXPECT_EQ(datagram.getHopLimit(), retDatagram->getHopLimit());
    EXPECT_EQ(datagram.getTrafficClass(), retDatagram->getTrafficClass());
    EXPECT_EQ(datagram.getFlowLabel(), retDatagram->getFlowLabel());
    ASSERT_EQ(1, retDatagram->getNumNextHeaders());
    cometos_v6::FollowingHeader* fh = retDatagram->getNextHeader();
    EXPECT_EQ(datagram.getNextHeader()->getHeaderType(), fh->getHeaderType());
    cometos_v6::UDPPacket* udp = (cometos_v6::UDPPacket*)fh;
    ASSERT_EQ(300, udp->getUpperLayerPayloadLength());
    const uint8_t* dp = udp->getData();
    for (uint16_t i = 0; i < 300; i++) {
        EXPECT_EQ((uint8_t)i, dp[i]);
    }
    bi->free();
}

TEST_F(assemblyBufferTest, addOutOfOrder) {

    AssemblyBuffer_t assemblyBuffer(&buf);

    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    cometos_v6::BufferInformation* bi = buf.getBuffer(300);

    for (uint16_t i = 0; i < 300; i++) {
        (*bi)[i] = i;
    }

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    cometos_v6::IPv6Request req(&datagram);

    cometos_v6::QueuePacket qp(&req,tag,bi,0);

    uint8_t maxSize = MAX_FRAGMENT_SIZE;
    cometos::Airframe frame;
    qp.createFrame(frame, maxSize, fm, dg);
    ASSERT_TRUE(fm.isFirstOfDatagram());

    uint8_t head;
    frame >> head;

    ASSERT_TRUE(cometos_v6::isFragmentationHeader1(head));

    uint16_nbo  fSize;
    uint16_nbo  fTag;
    uint8_t     tmp;

    frame >> tmp >> fTag;
    fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));

    frame >> head;
    cometos_v6::IPHCDecompressor* dcip =
            new cometos_v6::IPHCDecompressor(
                    head, frame, srcMAC, dstMAC, srcCtx, dstCtx);

    ASSERT_FALSE(NULL == dcip->getIPDatagram());

    cometos_v6::bufStatus_t stat = cometos_v6::BS_SUCCESS;

    cometos_v6::DatagramReassembly* fi = assemblyBuffer.addFirstFragment(
            srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), frame, dcip, stat);

    ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);

    ASSERT_FALSE(NULL == fi);

    ASSERT_FALSE(fi->isDone());

    bool notDone = true;
    cometos::Airframe aFrame, bFrame;
    uint8_t nMaxSize = MAX_FRAGMENT_SIZE;
    uint8_t     fOffset;
    qp.createFrame(aFrame, nMaxSize, fm, dg);
    ASSERT_FALSE(fm.isFirstOfDatagram());

    while (notDone) {
        nMaxSize = MAX_FRAGMENT_SIZE;
        qp.createFrame(bFrame, nMaxSize, fm, dg);

        if (bi->getSize() == 0) notDone = false;

        bFrame >> head;

        ASSERT_TRUE(cometos_v6::isFragmentationHeaderX(head));

        bFrame >> tmp >> fTag;
        fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));
        bFrame >> fOffset;

        fi = assemblyBuffer.addFragment(
                srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), fOffset, bFrame, stat);
        ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);
        ASSERT_FALSE(NULL == fi);
        EXPECT_EQ(72, fi->getContiguousSize());
    }
    aFrame >> head;

    ASSERT_TRUE(cometos_v6::isFragmentationHeaderX(head));

    aFrame >> tmp >> fTag;
    fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));
    aFrame >> fOffset;

    fi = assemblyBuffer.addFragment(
            srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), fOffset, aFrame, stat);
    ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);
    ASSERT_FALSE(NULL == fi);
    ASSERT_TRUE(fi->isDone());
    EXPECT_EQ(352, fi->getContiguousSize());

    bi->free();

    cometos_v6::IPv6Datagram* retDatagram = fi->getDatagram();
    ASSERT_TRUE(retDatagram != NULL);
    bi = fi->decapsulateBuffer();
    fi->free();
    EXPECT_EQ(datagram.src, retDatagram->src);
    EXPECT_EQ(datagram.dst, retDatagram->dst);
    EXPECT_EQ(datagram.getHopLimit(), retDatagram->getHopLimit());
    EXPECT_EQ(datagram.getTrafficClass(), retDatagram->getTrafficClass());
    EXPECT_EQ(datagram.getFlowLabel(), retDatagram->getFlowLabel());
    ASSERT_EQ(1, retDatagram->getNumNextHeaders());
    cometos_v6::FollowingHeader* fh = retDatagram->getNextHeader();
    EXPECT_EQ(datagram.getNextHeader()->getHeaderType(), fh->getHeaderType());
    cometos_v6::UDPPacket* udp = (cometos_v6::UDPPacket*)fh;
    ASSERT_EQ(300, udp->getUpperLayerPayloadLength());
    const uint8_t* dp = udp->getData();
    for (uint16_t i = 0; i < 300; i++) {
        EXPECT_EQ((uint8_t)i, dp[i]);
    }
    bi->free();
}

TEST_F(assemblyBufferTest, addWrong) {
    AssemblyBuffer_t assemblyBuffer(&buf);

    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    cometos_v6::BufferInformation* bi = buf.getBuffer(300);

    for (uint16_t i = 0; i < 300; i++) {
        (*bi)[i] = i;
    }

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    cometos_v6::IPv6Request req(&datagram);

    cometos_v6::QueuePacket qp(&req,tag,bi,0);

    uint8_t maxSize = MAX_FRAGMENT_SIZE;
    cometos::Airframe frame;
    qp.createFrame(frame, maxSize, fm, dg);

    uint8_t head;
    frame >> head;

    ASSERT_TRUE(cometos_v6::isFragmentationHeader1(head));
    uint16_nbo  fSize;
    uint16_nbo  fTag;
    uint8_t     tmp;

    frame >> tmp >> fTag;
    fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));

    frame >> head;
    cometos_v6::IPHCDecompressor* dcip =
            new cometos_v6::IPHCDecompressor(
                    head, frame, srcMAC, dstMAC, srcCtx, dstCtx);

    ASSERT_FALSE(NULL == dcip->getIPDatagram());

    cometos_v6::bufStatus_t stat = cometos_v6::BS_SUCCESS;

    cometos_v6::DatagramReassembly* fi = assemblyBuffer.addFirstFragment(
            srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), frame, dcip, stat);

    ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);

    ASSERT_FALSE(NULL == fi);

    ASSERT_FALSE(fi->isDone());

    bool notDone = true;
    while (notDone) {
        uint8_t nMaxSize = MAX_FRAGMENT_SIZE;
        cometos::Airframe nFrame;
        qp.createFrame(nFrame, nMaxSize, fm, dg);

        if (bi->getSize() == 0) notDone = false;

        nFrame >> head;

        ASSERT_TRUE(cometos_v6::isFragmentationHeaderX(head));

        nFrame >> tmp >> fTag;
        fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));
        uint8_t     fOffset;
        nFrame >> fOffset;

        fi = assemblyBuffer.addFragment(
                srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), fOffset - 1, nFrame, stat);
        ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);
        ASSERT_FALSE(NULL == fi);
    }

    bi->free();

    EXPECT_FALSE(fi->isDone());

    fi = assemblyBuffer.addFragment(
            srcMAC, 20, 20, 1, frame, stat);

    EXPECT_EQ(NULL, fi);
    ASSERT_EQ(cometos_v6::BS_FRAMGENT_NO_ENTRY, stat);
}

TEST_F(assemblyBufferTest, timeout) {
    const uint16_t BUF_SIZE = 300;
    AssemblyBuffer_t assemblyBuffer(&buf);

    cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
    cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

    cometos_v6::BufferInformation* bi = buf.getBuffer(BUF_SIZE);

    for (uint16_t i = 0; i < BUF_SIZE; i++) {
        (*bi)[i] = i;
    }

    udppacket.setData(bi->getContent(), bi->getSize());
    udppacket.generateChecksum(datagram.src, datagram.dst);

    cometos_v6::IPv6Request req(&datagram);


    cometos_v6::QueuePacket qp(&req,tag,bi,0);

    uint8_t maxSize = MAX_FRAGMENT_SIZE;
    cometos::Airframe frame;
    qp.createFrame(frame, maxSize, fm, dg);

    ASSERT_TRUE(fm.isFirstOfDatagram());
    uint8_t head;
    frame >> head;

    ASSERT_TRUE(cometos_v6::isFragmentationHeader1(head));
    uint16_nbo  fSize;
    uint16_nbo  fTag;
    uint8_t     tmp;

    frame >> tmp >> fTag;
    fSize = (((uint16_t)(head & 0x07)<<8)|(tmp));

    frame >> head;
    cometos_v6::IPHCDecompressor* dcip =
            new cometos_v6::IPHCDecompressor(
                    head, frame, srcMAC, dstMAC, srcCtx, dstCtx);

    ASSERT_FALSE(NULL == dcip->getIPDatagram());

    cometos_v6::bufStatus_t stat = cometos_v6::BS_SUCCESS;

    cometos_v6::DatagramReassembly* fi = assemblyBuffer.addFirstFragment(
            srcMAC, fTag.getUint16_t(), fSize.getUint16_t(), frame, dcip, stat);

    ASSERT_EQ(cometos_v6::BS_SUCCESS, stat);

    ASSERT_FALSE(NULL == fi);

    ASSERT_FALSE(fi->isDone());

    EXPECT_EQ(0, assemblyBuffer.tick());

    EXPECT_EQ(1, assemblyBuffer.tick());

    EXPECT_EQ(0, assemblyBuffer.tick());

    bi->free();
}

TEST_F(assemblyBufferTest, getAckBitmap) {

    cometos_v6::DatagramReassembly fragmentInformation;

    uint8_t seqNum = 0;
    fragmentInformation.storeSequenceNumberInAckBitmap(seqNum);
    EXPECT_EQ(0x80000000,
            fragmentInformation.getAckBitmap());

    seqNum = 31;
    fragmentInformation.storeSequenceNumberInAckBitmap(seqNum);
    EXPECT_EQ(0x80000001,
            fragmentInformation.getAckBitmap());

    seqNum = 32;
    fragmentInformation.storeSequenceNumberInAckBitmap(seqNum);
    EXPECT_EQ(0x80000001,
            fragmentInformation.getAckBitmap());
}

}

#endif /* ASSEMBLYBUFFER_UNITTEST_H_ */
