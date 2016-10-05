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

#ifndef QUEUEFRAME_UNITTEST_H_
#define QUEUEFRAME_UNITTEST_H_

#include "gtest/gtest.h"
#include "QueueFrame.h"
#include "omnetppDummyEnv.h"
#include "FragMetadata.h"

namespace {
  class queueFrameTest : public ::testing::Test {
  public:
      static const uint8_t MAX_SIZE = 80;

      static void SetUpTestCase() {
          OmnetppDummyEnv::setup();
      }

      void SetUp() {
          ipReq.data.datagram = &datagram;
          ipReq.data.dstMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)2);
          ipReq.data.srcMacAddress = cometos_v6::Ieee802154MacAddress((uint16_t)1);

          datagram.addHeader(&udppacket);
          datagram.setHopLimit(1);
          datagram.src.tryParse("1::1");
          datagram.dst.tryParse("1::2");
          datagram.setFlowLabel(0x12345);
          datagram.setTrafficClass(0x12);

          pi.set(&ipReq, 1, 2, 1280, MAX_SIZE);
      }
      void TearDown() {
          pi.decapsulateIPv6Datagram();
          datagram.removeHeader(0, false);
      }

  protected:
      cometos_v6::IPv6Request           ipReq;
      cometos_v6::IPv6Datagram          datagram;
      cometos_v6::UDPPacket             udppacket;
      cometos_v6::PacketInformation     pi;
      cometos_v6::LowpanBuffer<1280,15> buf;
      cometos::MacTxInfo                info;
      cometos_v6::LowpanFragMetadata    fm;
      const cometos_v6::IPv6Datagram*   dg;
  };

  TEST_F(queueFrameTest, getFirst) {
      cometos_v6::BufferInformation* bi = buf.getBuffer(1280);
      cometos_v6::QueueFrame    qf(&pi, bi, 0, 0,false);

      udppacket.setData(bi->getContent(), 1280);

      EXPECT_FALSE(qf.canBeDeleted());
//      EXPECT_EQ(&pi, qf.getDirectPacket());
      EXPECT_EQ(&pi, qf.getDirectPacket());
      EXPECT_FALSE(qf.isSent());

      cometos::Airframe frame;

      qf.createFrame(frame, MAX_SIZE, fm, dg);

      EXPECT_EQ(75, frame.getLength());
      EXPECT_TRUE(qf.isSent());

      uint8_t checkContent[43] = {0xC5, 0x00, 0x00, 0x02,
              0x61, 0x00, 0x12, 0x01, 0x23, 0x45, 0x11,
              0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
              0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};

      for (uint8_t i = 0; i < 43; i++) {
//          std::cout << "i=" << (int) i << endl;
          uint8_t b;
          frame >> b;
          EXPECT_EQ(checkContent[i], b);
      }

      EXPECT_EQ(cometos_v6::QueueObject::QUEUE_DELETE_OBJECT, qf.response(true, info));
      EXPECT_FALSE(pi.isFree());
  }

  TEST_F(queueFrameTest, getSecond) {
      cometos_v6::BufferInformation* bi = buf.getBuffer(35);
      cometos_v6::QueueFrame    qf(&pi, bi, 0, MAX_SIZE, false);

      cometos::Airframe frame;

      qf.createFrame(frame, MAX_SIZE, fm, dg);

      EXPECT_EQ(40, frame.getLength());
      EXPECT_TRUE(qf.isSent());

      uint8_t checkContent[5] = {0xE5, 0x00, 0x00, 0x02, 80};

      for (uint8_t i = 0; i < 5; i++) {
          uint8_t b;
          frame >> b;
          EXPECT_EQ(checkContent[i], b);
      }

      EXPECT_EQ(cometos_v6::QueueObject::QUEUE_DELETE_OBJECT, qf.response(true, info));
      EXPECT_FALSE(pi.isFree());

  }

  TEST_F(queueFrameTest, negativeResponse) {
      cometos_v6::BufferInformation* bi = buf.getBuffer(35);
      cometos_v6::QueueFrame    qf(&pi, bi, 0, MAX_SIZE, false);

      cometos::Airframe frame;

      qf.createFrame(frame, MAX_SIZE, fm, dg);

      EXPECT_EQ(40, frame.getLength());
      EXPECT_TRUE(qf.isSent());

      uint8_t checkContent[5] = {0xE5, 0x00, 0x00, 0x02, 80};

      for (uint8_t i = 0; i < 5; i++) {
          uint8_t b;
          frame >> b;
          EXPECT_EQ(checkContent[i], b);
      }

      pi.decapsulateIPv6Datagram();
      EXPECT_EQ(cometos_v6::QueueObject::QUEUE_DELETE_SIMILAR, qf.response(false, info));
      EXPECT_TRUE(pi.isFree());

      cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
      cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

      pi.set(&ipReq, 1, 2, 1280, MAX_SIZE);

  }

  TEST_F(queueFrameTest, lastFrame) {
      cometos_v6::BufferInformation* bi = buf.getBuffer(40);
      cometos_v6::QueueFrame    qf(&pi, bi, 0, 0x9B, true);

      cometos::Airframe frame;

      qf.createFrame(frame, MAX_SIZE, fm, dg);

      EXPECT_EQ(45, frame.getLength());
      EXPECT_TRUE(qf.isSent());

      uint8_t checkContent[5] = {0xE5, 0x00, 0x00, 0x02, 0x9B};

      for (uint8_t i = 0; i < 5; i++) {
          uint8_t b;
          frame >> b;
          EXPECT_EQ(checkContent[i], b);
      }

      pi.decapsulateIPv6Datagram();
      cometos_v6::QueueObject::response_t ret = qf.response(true, info);
      ASSERT_TRUE(ret == cometos_v6::QueueObject::QUEUE_DELETE_OBJECT || cometos_v6::QueueObject::QUEUE_PACKET_FINISHED);
      EXPECT_TRUE(pi.isFree());

      cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
      cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

      pi.set(&ipReq, 1, 2, 1280, MAX_SIZE);

  }

  TEST_F(queueFrameTest, getRCValues) {
      for (uint8_t offset = 0; offset < 255; offset++) {
          cometos_v6::BufferInformation* bi = buf.getBuffer(1);
          cometos_v6::QueueFrame    qf(&pi, bi, 0, offset, false);
          EXPECT_EQ(offset, qf.getSRCValue());
          EXPECT_EQ((offset/(1280>>3)), qf.getPRCValue());
      }
  }

}


#endif /* QUEUEFRAME_UNITTEST_H_ */
