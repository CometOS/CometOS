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

#ifndef FIFOLOWPANQUEUE_H_
#define FIFOLOWPANQUEUE_H_


#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lowpan-macros.h"
#include "QueueFrame.h"
#include "omnetppDummyEnv.h"
#include "LowpanQueue.h"
#include "FifoLowpanQueue.h"
#include "DataResponse.h"
#include "DataRequest.h"
#include "testutil.h"
#include "FragMetadata.h"


using ::testing::ElementsAreArray;

namespace {
  class FifoLowpanQueueTest : public ::testing::Test {
  public:
      static const uint8_t MAX_SIZE = 80;
      static const uint8_t Q_MAX_SIZE = 10;
      static const uint16_t DG_SIZE = 400;
      static const uint16_t BUF_SIZE = DG_SIZE
              - cometos_v6::IPv6Datagram::IPV6_HEADER_SIZE
              - cometos_v6::UDPPacket::UDP_HEADER_SIZE;
      static const uint8_t PAYLOAD_FOLLOWING_FRAG = 72;
      static const uint8_t PAYLOAD_FIRST_FRAG = 24;

      FifoLowpanQueueTest() :
          theQueue(MAX_SIZE)
      {}

      static void SetUpTestCase() {
          OmnetppDummyEnv::setup();
      }

      void SetUp() {

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

          queue = &theQueue;

          pi1.set(&ipReq1, 1, 2, DG_SIZE, MAX_SIZE);
      }
      void TearDown() {
          pi1.decapsulateIPv6Datagram();
          datagram1.removeHeader(0, false);
      }

  protected:
      cometos_v6::LowpanBuffer<1280,15> buf;
      cometos_v6::IPv6Request           ipReq1;
      cometos_v6::IPv6Datagram          datagram1;
      cometos_v6::UDPPacket             udppacket1;
      cometos_v6::PacketInformation     pi1;
      cometos::MacTxInfo                info;
      cometos_v6::LowpanQueue*          queue;
      cometos_v6::FifoLowpanQueue<Q_MAX_SIZE> theQueue;
      cometos_v6::LowpanFragMetadata    fm;
      const cometos_v6::IPv6Datagram*   dg;

  };

  TEST_F(FifoLowpanQueueTest, getFirst) {
      uint8_t checkContent[52] = {0xC1, 0x90, 0x00, 0x02,
          0x60, 0x00, 0x12, 0x01, 0x23, 0x45, 0x11, 0x22,
          0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
          0xde, 0xad, 0xbe, 0xef, 0x01, 0x98, 0x00, 0x00
      };
      uint8_t checkContent2[5] = {0xE1, 0x90, 0x00, 0x02, 0x03};

      cometos_v6::BufferInformation* bi = buf.getBuffer(PAYLOAD_FIRST_FRAG);
      cometos_v6::BufferInformation* bi2 = buf.getBuffer(PAYLOAD_FOLLOWING_FRAG);
      cometos_v6::BufferInformation* bi3 = buf.getBuffer(PAYLOAD_FOLLOWING_FRAG);
      udppacket1.setData(bi->getContent(), DG_SIZE);
      cometos_v6::QueueFrame* qf1 = new cometos_v6::QueueFrame(&pi1, bi, 0, 0, false);
      cometos_v6::QueueFrame* qf2 = new cometos_v6::QueueFrame(&pi1, bi2, 0, 3, false);
      cometos_v6::QueueFrame* qf3 = new cometos_v6::QueueFrame(&pi1, bi3, 0, 11, false);
      queue->add(qf1);
      queue->add(qf2);
      queue->add(qf3);

      uint8_t expectedSize = 76; // 40 IP + 8 UDP + 3*8=24 Data + 4 FragHeader

      cometos::DataRequest* req = queue->getNext(fm, dg);
      {
          cometos::Airframe& frame = req->getAirframe();

          EXPECT_EQ(fm.isFirstOfDatagram(), true);

          EXPECT_EQ(frame.getLength(), expectedSize);

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

  TEST_F(FifoLowpanQueueTest, lastFrame) {
      uint8_t numFragsAfterFirst = ((BUF_SIZE - PAYLOAD_FIRST_FRAG)-1) / PAYLOAD_FOLLOWING_FRAG + 1;
      uint8_t numFragsTotal = numFragsAfterFirst + 1;
      uint8_t lastFragSize = (BUF_SIZE - PAYLOAD_FIRST_FRAG - 1) % PAYLOAD_FOLLOWING_FRAG + 1;
      cometos_v6::BufferInformation* bi[numFragsTotal];
      bi[0] = buf.getBuffer(PAYLOAD_FIRST_FRAG);
      uint8_t i=1;
      for (; i < numFragsAfterFirst; i++) {
          bi[i] = buf.getBuffer(PAYLOAD_FOLLOWING_FRAG);
      }
      bi[i] = buf.getBuffer(lastFragSize);


      cometos_v6::QueueFrame* qf[numFragsTotal];
      qf[0] = new cometos_v6::QueueFrame(&pi1, bi[0], 0, 0, false);
      queue->add(qf[0]);
      for (i = 1; i< numFragsAfterFirst; i++) {
          qf[i] = new cometos_v6::QueueFrame(
                  &pi1,
                  bi[i],
                  0,
                  PAYLOAD_FIRST_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT + (i-1) * cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT,
                  false);
          queue->add(qf[i]);
      }
      qf[i] = new cometos_v6::QueueFrame(
              &pi1,
              bi[i],
              0,
              PAYLOAD_FIRST_FRAG / cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT + (i-1) * cometos_v6::LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT,
              true);
      queue->add(qf[i]);

      cometos::DataRequest* req;

      for (i=0; i<numFragsTotal-1; i++) {
          req = queue->getNext(fm, dg);

          delete req;
          req=NULL;
          queue->response(new cometos::DataResponse(true));
      }

      req=queue->getNext(fm, dg);
      cometos::Airframe & frame = req->getAirframe();

      EXPECT_FALSE(fm.isFirstOfDatagram());
      EXPECT_EQ(frame.getLength(), lastFragSize + SIZE_OF_LOWPAN_SUBSEQUENT_HEADERS);
      EXPECT_EQ(queue->response(new cometos::DataResponse(true)), cometos_v6::lowpanQueueResponse_t::LOWPANQUEUE_PACKET_FINISHED);
      EXPECT_TRUE(pi1.isFree());

  }


}


#endif /* FIFOLOWPANQUEUE_H_ */
