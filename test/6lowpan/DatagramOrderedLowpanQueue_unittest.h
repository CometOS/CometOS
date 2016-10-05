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

#ifndef DATAGRAMORDEREDLOWPANQUEUE_H_
#define DATAGRAMORDEREDLOWPANQUEUE_H_

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lowpan-macros.h"
#include "QueueFrame.h"
#include "omnetppDummyEnv.h"
#include "LowpanQueue.h"
#include "DgOrderedLowpanQueue.h"
#include "DataResponse.h"
#include "DataRequest.h"
#include "testutil.h"
#include "QueuePacket.h"

#define OUT "[--OUT-----] "

using ::testing::ElementsAreArray;

namespace {
class DatagramOrderedLowpanQueueTest : public ::testing::Test {
public:
    static const uint8_t MAX_SIZE = 80;
    static const uint8_t Q_MAX_SIZE = 10;


    /* size including IPv6/UDP headers */
    static const uint16_t DG1_SIZE = 400;

    /* payload size */
    static const uint16_t BUF1_SIZE = DG1_SIZE
          - cometos_v6::IPv6Datagram::IPV6_HEADER_SIZE
          - cometos_v6::UDPPacket::UDP_HEADER_SIZE;


    static const uint16_t QF1_INCOMING_TAG = 1;
    static const uint16_t QF1_OUTGOING_TAG = 2;
    static const uint8_t QF1_EXP_FIRST_FRAG_SIZE = 76;
    static const uint8_t QF1_EXP_FOLLOW_FRAG_SIZE = 77;

    static const uint8_t DG2_SIZE = 160;
    static const uint8_t BUF2_SIZE = DG2_SIZE
            - cometos_v6::IPv6Datagram::IPV6_HEADER_SIZE
            - cometos_v6::UDPPacket::UDP_HEADER_SIZE;

    static const uint16_t QF2_INCOMING_TAG = 2;
    static const uint16_t QF2_OUTGOING_TAG = 3;
    static const uint8_t QF2_EXP_FIRST_FRAG_SIZE = 76;
    static const uint8_t QF2_EXP_FOLLOW_FRAG_SIZE = 77;

    static const uint16_t QP_TAG = 3;
    static const uint16_t QP_SIZE = 20;

    static const uint8_t PAYLOAD_FOLLOWING_FRAG = 72;
    static const uint8_t PAYLOAD_FIRST_FRAG = 24;

    DatagramOrderedLowpanQueueTest() :
        theQueue(Q_MAX_SIZE, MAX_SIZE)
    {}

    static void SetUpTestCase() {
        OmnetppDummyEnv::setup();
    }

    void fillBuf(cometos_v6::BufferInformation**& buffer,
                 uint8_t numFragsTotal,
                 uint8_t numFragsAfterFirst,
                 uint8_t lastFragSize) {
        buffer = new cometos_v6::BufferInformation*[numFragsTotal];
        buffer[0] = buf.getBuffer(PAYLOAD_FIRST_FRAG);
//        std::cout << OUT << "created buffer@" << buffer[0] << "; size=" << (int)buffer[0]->getSize() << std::endl;
        uint8_t i=1;
        for (; i < numFragsAfterFirst; i++) {
            buffer[i] = buf.getBuffer(PAYLOAD_FOLLOWING_FRAG);
//            std::cout << OUT  << "created buffer@" << buffer[i] << "; size=" << (int)buffer[i]->getSize() << std::endl;
        }
        buffer[i] = buf.getBuffer(lastFragSize);
//        std::cout << OUT  << "created buffer@" << buffer[i] << "; size=" << (int)buffer[i]->getSize() << std::endl;
    }

    void createFrames(cometos_v6::QueueFrame**& qf,
                      cometos_v6::PacketInformation* pi,
                      cometos_v6::BufferInformation** bi,
                      uint8_t numFragsTotal,
                      uint8_t numFragsAfterFirst) {
        qf = new cometos_v6::QueueFrame*[numFragsTotal];
        uint8_t i = 0;
        qf[0] = new cometos_v6::QueueFrame(pi, bi[0], 0, 0, false);
        for (i = 1; i< numFragsAfterFirst; i++) {
            qf[i] = new cometos_v6::QueueFrame(
                  pi,
                  bi[i],
                  0,
                  PAYLOAD_FIRST_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT
                      + (i-1) * PAYLOAD_FOLLOWING_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT,
                  false);
//            std::cout << OUT  << "new qf@" << *qf << "[" << (int) i
//                                  << "]=" << qf[i] <<"; bi[" << (int) i << "]=" << (int) bi[i]->getSize() << std::endl;
        }
        qf[i] = new cometos_v6::QueueFrame(
                  pi,
                  bi[i],
                  0,
                  PAYLOAD_FIRST_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT
                      + (i-1) * PAYLOAD_FOLLOWING_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT,
                  true);
//        std::cout << OUT  << "new qf@" << *qf << "[" << (int) i
//                                      << "]=" << qf[i] <<"; bi[" << (int) i << "]=" << (int) bi[i]->getSize() << std::endl;
    }

    void SetUp() {
        // setup first in-transit DG
        ipReq1.data.datagram = &datagram1;
        ipReq1.data.dstMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)2);
        ipReq1.data.srcMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)1);

        udppacket1.setSrcPort(0xdead);
        udppacket1.setDestPort(0xbeef);
        datagram1.addHeader(&udppacket1);
        datagram1.setHopLimit(34);
        datagram1.src.tryParse("1::1");
        datagram1.dst.tryParse("1::2");
        datagram1.setFlowLabel(0x12345);
        datagram1.setTrafficClass(0x12);

        pi1.set(&ipReq1, QF1_INCOMING_TAG, QF1_OUTGOING_TAG, DG1_SIZE, MAX_SIZE);

        // setup second in-transit DG
        ipReq2.data.datagram = &datagram2;
        ipReq2.data.dstMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)2);
        ipReq2.data.srcMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)1);

        datagram2.setHopLimit(10);
        datagram2.src.tryParse("1::5");
        datagram2.dst.tryParse("1::6");
        datagram2.setFlowLabel(0x54321);
        datagram2.setTrafficClass(0x21);

        pi2.set(&ipReq1, QF2_INCOMING_TAG, QF2_OUTGOING_TAG, DG2_SIZE, MAX_SIZE);

        // setup another req/datagram for a queuePacket
        qpReq.data.datagram = &qpDg;
        qpReq.data.dstMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)4);
        qpReq.data.srcMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)3);

        qpUdp.setSrcPort(0xeeff);
        qpUdp.setDestPort(0xaabb);

        qpDg.addHeader(&qpUdp);
        qpDg.setHopLimit(1);
        qpDg.src.tryParse("1::3");
        qpDg.dst.tryParse("1::4");
        qpDg.setFlowLabel(0x12345);
        qpDg.setTrafficClass(0x12);

        // let queue interface variable point to the "real" queue
        queue = &theQueue;

        // create QueueFrames and QueuePacket
        numFragsAfterFirst1 = ((BUF1_SIZE - PAYLOAD_FIRST_FRAG)-1) / PAYLOAD_FOLLOWING_FRAG + 1;
        numFragsTotal1 = numFragsAfterFirst1 + 1;
        lastFragSize1 = (BUF1_SIZE - PAYLOAD_FIRST_FRAG - 1) % PAYLOAD_FOLLOWING_FRAG + 1;

        numFragsAfterFirst2 = ((BUF2_SIZE - PAYLOAD_FIRST_FRAG)-1) / PAYLOAD_FOLLOWING_FRAG + 1;
        numFragsTotal2 = numFragsAfterFirst2 + 1;
        lastFragSize2 = (BUF2_SIZE - PAYLOAD_FIRST_FRAG - 1) % PAYLOAD_FOLLOWING_FRAG +1;

        fillBuf(bi1, numFragsTotal1, numFragsAfterFirst1, lastFragSize1);
        fillBuf(bi2, numFragsTotal2, numFragsAfterFirst2, lastFragSize2);

        udppacket1.setData(bi1[0]->getContent(), DG1_SIZE);

        createFrames(qf1, &pi1, bi1, numFragsTotal1, numFragsAfterFirst1);
        createFrames(qf2, &pi2, bi2, numFragsTotal2, numFragsAfterFirst2);

        qpBi = buf.getBuffer(QP_SIZE);
        qp = new cometos_v6::QueuePacket(
                &qpReq,
                QP_TAG,
                qpBi,
                0);
    }
    void TearDown() {
        pi1.decapsulateIPv6Datagram();
        datagram1.removeHeader(0, false);
        qpDg.removeHeader(0, false);
        delete[] bi1;
        delete[] bi2;
    }

protected:
    cometos_v6::LowpanBuffer<2500,15> buf;

    // first in-transit DG
    cometos_v6::IPv6Request           ipReq1;
    cometos_v6::IPv6Datagram          datagram1;
    cometos_v6::UDPPacket             udppacket1;
    cometos_v6::PacketInformation     pi1;

    // second in-transit DG
    cometos_v6::IPv6Request           ipReq2;
    cometos_v6::IPv6Datagram          datagram2;
    cometos_v6::PacketInformation     pi2;

    // QueuePacket to further confuse the queue
    cometos_v6::IPv6Request           qpReq;
    cometos_v6::IPv6Datagram          qpDg;
    cometos_v6::UDPPacket             qpUdp;

    // the queues to test
    cometos_v6::LowpanQueue*          queue;
    cometos_v6::DgOrderedLowpanQueue theQueue;

    // some helper variables to create buffer objects and queueFrames
    uint8_t numFragsAfterFirst1;
    uint8_t numFragsTotal1;
    uint8_t lastFragSize1;

    uint8_t numFragsAfterFirst2;
    uint8_t numFragsTotal2;
    uint8_t lastFragSize2;

    cometos_v6::BufferInformation** bi1;
    cometos_v6::BufferInformation** bi2;
    cometos_v6::QueueFrame** qf1;
    cometos_v6::QueueFrame** qf2;

    cometos_v6::BufferInformation* qpBi;
    cometos_v6::QueuePacket* qp;

    cometos_v6::LowpanFragMetadata fm;
    const cometos_v6::IPv6Datagram* dg;
};


TEST_F(DatagramOrderedLowpanQueueTest, singleDg) {
    uint8_t checkContent[52] = {0xC1, 0x90, 0x00, 0x02,
        0x60, 0x00, 0x12, 0x01, 0x23, 0x45, 0x11, 0x22,
        0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
        0xde, 0xad, 0xbe, 0xef, 0x01, 0x98, 0x00, 0x00
    };
    uint8_t checkContent2[5] = {0xE1, 0x90, 0x00, 0x02, 0x03};

    queue->add(qf1[0]);
    queue->add(qf1[1]);
    queue->add(qf1[2]);

    cometos::DataRequest* req = queue->getNext(fm, dg);
    {
        cometos::Airframe& frame = req->getAirframe();

        EXPECT_EQ(fm.isFirstOfDatagram(), true);

        EXPECT_EQ(frame.getLength(), QF1_EXP_FIRST_FRAG_SIZE);

        CREATE_REVERSED_FRAME(reverseData, frame);
        EXPECT_THAT(std::vector<uint8_t>(reverseData, reverseData + 52), ElementsAreArray(checkContent));

        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
        EXPECT_FALSE(pi1.isFree());

        delete(req);
        req = NULL;
    }

    {
        // second frame
        req = queue->getNext(fm, dg);
        cometos::Airframe& frame = req->getAirframe();

        EXPECT_EQ(fm.isFirstOfDatagram(), false);
        EXPECT_EQ(frame.getLength(), 77);

        CREATE_REVERSED_FRAME(reverseData, frame);

        EXPECT_THAT(std::vector<uint8_t>(reverseData, reverseData + 5), ElementsAreArray(checkContent2));
        EXPECT_FALSE(pi1.isFree());
        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
        delete(req);
        req = NULL;
    }

    {
        // third frame
        req  = queue->getNext(fm, dg);

        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_DROPPED, queue->response(new cometos::DataResponse(false)));
        EXPECT_TRUE(pi1.isFree());

        delete(req);
        req = NULL;
    }
}

TEST_F(DatagramOrderedLowpanQueueTest, emptyInOrderDg) {

    // check if the queue correctly handles situations, which leave the
    // internal queue of an InOrderDg empty
    queue->add(qf1[0]);
    cometos::DataRequest* req;
    cometos::Airframe* frame;

    EXPECT_TRUE(queue->hasNext());

    req = queue->getNext(fm, dg);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    frame = req->decapsulateAirframe();
    delete(req);
    EXPECT_EQ(QF1_EXP_FIRST_FRAG_SIZE, frame->getLength());
    {
        uint8_t checkHeader[4] = {0xC1, 0x90, QF1_OUTGOING_TAG >> 8, QF1_OUTGOING_TAG & 0xFF};
        CREATE_REVERSED_FRAME(reverseData, *frame);
        EXPECT_THAT(std::vector<uint8_t>(reverseData, reverseData+4), ElementsAreArray(checkHeader));
        EXPECT_TRUE(queue->hasNext());
        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
        EXPECT_FALSE(queue->hasNext());
        EXPECT_EQ(queue->getQueueSize(), 1);
    }


    // add and retrieve rest of datagram
    for (uint8_t i=1; i < numFragsTotal1-1; i++) {
        queue->add(qf1[i]);
        EXPECT_TRUE(queue->hasNext());
        req=queue->getNext(fm, dg);
        EXPECT_FALSE(fm.isFirstOfDatagram());
        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
        EXPECT_EQ(1, queue->getQueueSize());
        EXPECT_FALSE(queue->hasNext());
        delete(req);
    }
    queue->add(qf1[numFragsTotal1 - 1]);
    EXPECT_TRUE(queue->hasNext());
    req=queue->getNext(fm, dg);
    EXPECT_FALSE(fm.isFirstOfDatagram());
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED, queue->response(new cometos::DataResponse(true)));
    EXPECT_EQ(0, queue->getQueueSize());
    delete(req);
}

TEST_F(DatagramOrderedLowpanQueueTest, mixedOrder) {
    cometos::DataRequest* req;
    cometos::Airframe* frame;

    queue->add(qf1[0]);
    queue->add(qp);
    EXPECT_TRUE(queue->hasNext());

    // now, let the first queue frame generate a frame
    req = queue->getNext(fm, dg);
    delete(req);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
    EXPECT_FALSE(queue->hasNext());
    queue->add(qf1[1]);
    EXPECT_TRUE(queue->hasNext());

    // add another complete datagram
    for (uint8_t i = 0; i < numFragsTotal2; i++) {
        queue->add(qf2[i]);
    }

    // add the rest of the original datagram
    for (uint8_t i=2; i < numFragsTotal1; i++) {
//        std::cout << OUT << "adding qf@" << qf1[i] << "[" << (int) i << "]; bi@" << qf1[i]->buffer << "=" << (int) qf1[i]->buffer->getSize() << std::endl;
        queue->add(qf1[i]);
    }


    uint8_t firstOffset;
    uint8_t offset;
    uint8_t i;
    for (i = 1; i < numFragsTotal1-1; i++) {
        EXPECT_TRUE(queue->hasNext());
        req=queue->getNext(fm, dg);
        EXPECT_FALSE(fm.isFirstOfDatagram());
        frame = req->decapsulateAirframe();
        EXPECT_EQ(77, frame->getLength());
        firstOffset = PAYLOAD_FIRST_FRAG >> cometos_v6::LOWPAN_FRAG_OFFSET_SHIFT;
        offset = (firstOffset + (((i-1)*PAYLOAD_FOLLOWING_FRAG)>>cometos_v6::LOWPAN_FRAG_OFFSET_SHIFT));
        uint8_t checkContent[5] = {0xE1, 0x90, 0x00, 0x02, offset};
        CREATE_REVERSED_FRAME(fData, *frame);
        EXPECT_THAT(std::vector<uint8_t>(fData, fData+5), ElementsAreArray(checkContent));
        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
        delete(frame);
        delete(req);
        req=NULL;
        frame=NULL;
    }

    offset = (firstOffset + (((i-1)*PAYLOAD_FOLLOWING_FRAG)>>cometos_v6::LOWPAN_FRAG_OFFSET_SHIFT));

    EXPECT_TRUE(queue->hasNext());
    EXPECT_EQ(3, queue->getQueueSize());
    req=queue->getNext(fm, dg);
    EXPECT_FALSE(fm.isFirstOfDatagram());
    frame = req->decapsulateAirframe();
    EXPECT_EQ(45, frame->getLength());
    uint8_t checkContent[5] = {0xE1, 0x90, 0x00, 0x02, offset};
    {
        CREATE_REVERSED_FRAME(fData, *frame);
        EXPECT_THAT(std::vector<uint8_t>(fData, fData+5), ElementsAreArray(checkContent));
    }
    delete(frame);
    delete(req);
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED, queue->response(new cometos::DataResponse(true)));

    EXPECT_EQ(2, queue->getQueueSize());
    EXPECT_TRUE(queue->hasNext());

    req=queue->getNext(fm, dg);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED, queue->response(new cometos::DataResponse(true)));
    delete(req);
    EXPECT_EQ(1, queue->getQueueSize());

}


TEST_F(DatagramOrderedLowpanQueueTest, InOrderAsSecond) {
    queue->add(qp);
    for (uint8_t i=0; i < numFragsTotal1-1; i++) {
        queue->add(qf1[i]);
    }

    // first remove the previously added queue packet
    delete queue->getNext(fm, dg);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    EXPECT_TRUE(queue->hasNext());
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED, queue->response(new cometos::DataResponse(true)));

    // now get queue frames one after the other
    for (uint8_t i=0; i < numFragsTotal1-1; i++) {
        delete queue->getNext(fm, dg);
        EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_IN_PROGRESS, queue->response(new cometos::DataResponse(true)));
    }
    EXPECT_EQ(1, queue->getQueueSize());
    EXPECT_FALSE(queue->hasNext());
}

TEST_F(DatagramOrderedLowpanQueueTest, removeInOrderDgAfterFailure) {

    queue->add(qp);
    for (uint8_t i=0; i < numFragsTotal2; i++) {
        queue->add(qf2[i]);
    }

    for (uint8_t i=0; i < numFragsTotal1; i++) {
        queue->add(qf1[i]);
    }

    delete queue->getNext(fm, dg);
    queue->response(new cometos::DataResponse(true));

    delete queue->getNext(fm, dg);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    queue->response(new cometos::DataResponse(true));
    delete queue->getNext(fm, dg);
    EXPECT_FALSE(fm.isFirstOfDatagram());
    delete queue->getNext(fm, dg);
    EXPECT_EQ(cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_DROPPED, queue->response(new cometos::DataResponse(false)));
    EXPECT_EQ(1, queue->getQueueSize());
    delete queue->getNext(fm, dg);
    EXPECT_TRUE(fm.isFirstOfDatagram());
    queue->response(new cometos::DataResponse(false));
}


const uint8_t DatagramOrderedLowpanQueueTest::QF1_EXP_FIRST_FRAG_SIZE;

} // namespace

#endif /* DATAGRAMORDEREDLOWPANQUEUE_H_ */
