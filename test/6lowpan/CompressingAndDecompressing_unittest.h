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

#ifndef COMPRESSINGANDDECOMPRESSING_UNITTEST_H_
#define COMPRESSINGANDDECOMPRESSING_UNITTEST_H_

#include "gtest/gtest.h"
#include "IPHCCompressor.h"
#include "LowpanBuffer.h"
#include "IPHCDecompressor.h"
#include "omnetppDummyEnv.h"

namespace {
  class compressingAndDecompressing : public ::testing::Test {
  public:
      static void SetUpTestCase() {
          OmnetppDummyEnv::setup();
      }
      void SetUp() {
      }
      void TearDown() {
      }

      compressingAndDecompressing() :
          buf(200, 2)
      {}

  protected:
//      cometos_v6::LowpanBuffer<200,2>       buf;
      cometos_v6::DynLowpanBuffer   buf;
      cometos_v6::IPv6Datagram  datagram;
      IPv6Context srcCtx[16];
      IPv6Context dstCtx[16];
  };

  TEST_F(compressingAndDecompressing, IP) {
      cometos_v6::BufferInformation* bi = buf.getBuffer(20);
      cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
      cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);
      cometos_v6::UDPPacket udppacket;
      datagram.addHeader(&udppacket);

      const char* srcAddr[4] = { "1::1", "1::2", "fe80::1", "fe80::2" };
      const char* dstAddr[6] = { "1::2", "1::1", "fe80::2", "fe80::1", "ff01::2", "ff02::2" };

     // uint8_t expSize = 29;

      for (uint8_t hlim = 1; hlim > 0; hlim++) {
          datagram.setHopLimit(hlim);
          for (uint8_t sc = 0; sc < 4; sc++) {
              datagram.src.tryParse(srcAddr[sc]);
              for (uint8_t dc = 0; dc < 6; dc++) {
                  datagram.dst.tryParse(dstAddr[dc]);
                  const uint32_t fll[2] = {0, 0x12345};
                  for (uint8_t i = 0; i < 2; i++) {
                      const uint32_t fl = fll[i];
                      const uint8_t tcl[4] = {0x00, 0x01, 0x80, 0x81};
                      for (uint8_t j = 0; j < 4; j++) {
                          const uint8_t tc = tcl[j];

                          datagram.setFlowLabel(fl);
                          datagram.setTrafficClass(tc);

                          udppacket.setData(bi->getContent(), bi->getSize());
                          udppacket.generateChecksum(datagram.src, datagram.dst);

                          cometos_v6::IPHCCompressor cd(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
                          cometos::Airframe frame;

                          uint8_t maxSize = 80;
                          uint16_t posInBuffer = 0;
                          uint16_t size = cd.streamDatagramToFrame(frame, maxSize, bi, posInBuffer);

                          EXPECT_EQ(datagram.getCompleteHeaderLength() + datagram.getUpperLayerPayloadLength(), size);
                          //EXPECT_EQ(80 - expSize, maxSize);
                          //EXPECT_EQ(expSize, frame.getLength());

                          uint8_t head;
                          frame >> head;

                          cometos_v6::IPHCDecompressor dc(head, frame, srcMAC, dstMAC, srcCtx, dstCtx);

                          ASSERT_FALSE(NULL == dc.getIPDatagram());


//                          cout << "from: " << datagram.src.str()
//                                  << " to: " << datagram.dst.str()
//                                  << " FL: " << fl
//                                  << " TC: " << (int)tc
//                                  << " HL: " << (int)hlim << endl;


                          EXPECT_EQ(datagram.src, dc.getIPDatagram()->src);
                          EXPECT_EQ(datagram.dst, dc.getIPDatagram()->dst);
                          EXPECT_EQ(datagram.getFlowLabel(), dc.getIPDatagram()->getFlowLabel());
                          EXPECT_EQ(datagram.getTrafficClass(), dc.getIPDatagram()->getTrafficClass());
                          EXPECT_EQ(datagram.getHopLimit(), dc.getIPDatagram()->getHopLimit());
                      }
                  }
              }
          }
      }

      datagram.removeHeader(0, false);

      bi->free();
  }

  TEST_F(compressingAndDecompressing, UDP) {
      const uint8_t PAYLOAD_SIZE = 20;
      cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
      cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);
      cometos_v6::UDPPacket udppacket;
      datagram.addHeader(&udppacket);
      datagram.setHopLimit(1);
      datagram.src.tryParse("1::1");
      datagram.dst.tryParse("1::2");

      datagram.setFlowLabel(0x12345);
      datagram.setTrafficClass(0x12);

      for (uint16_t srcPort = 0xF001; srcPort < 0xF110; srcPort += 8) {
          for (uint16_t dstPort = 0xF001; dstPort < 0xF110; dstPort += 8) {
              LOG_DEBUG("srcPort=" << cometos::hex << srcPort << "|dstPort=" << dstPort);
              cometos_v6::BufferInformation* bi = buf.getBuffer(PAYLOAD_SIZE);
              udppacket.setData(bi->getContent(), bi->getSize());

              udppacket.setSrcPort(srcPort);
              udppacket.setDestPort(dstPort);
              udppacket.generateChecksum(datagram.src, datagram.dst);

              cometos_v6::IPHCCompressor cd(&datagram, srcMAC, dstMAC, cometos_v6::FollowingHeader::NoNextHeaderNumber);
              cometos::Airframe frame;

              uint8_t maxSize = 80;
              uint16_t posInBuffer = 0;
              uint16_t size = cd.streamDatagramToFrame(frame, maxSize, bi, posInBuffer);

              bi->free();

              uint8_t expSize = 61;  // 38 byte IPv6, 1 NHC + 2 CHKSUM + X Ports, 20 bytes UDP payload
              uint8_t lenPorts;

              if (((srcPort & 0xFFF0) == 0xF0B0) && ((dstPort & 0xFFF0) == 0xF0B0)) {
                  lenPorts = 1;
              } else if ( ((srcPort & 0xFF00) == 0xF000) || ((dstPort & 0xFF00) == 0xF000)) {
                  lenPorts = 3;
              } else {
                  lenPorts = 4;
              }

              expSize += lenPorts;

              EXPECT_EQ(datagram.getCompleteHeaderLength()+datagram.getUpperLayerPayloadLength(), size);
              EXPECT_EQ(expSize, frame.getLength());

              uint8_t head;
              frame >> head;

              cometos_v6::IPHCDecompressor dc(head, frame, srcMAC, dstMAC, srcCtx, dstCtx);
              cometos_v6::BufferInformation* bo = buf.getBuffer(PAYLOAD_SIZE);
              cometos_v6::FirstFragBufInfo ret = dc.compressedNext(frame, bo);
              EXPECT_EQ(0, frame.getLength());
              for (uint8_t i = 0; frame.getLength() > 0; i++) {
                  frame >> (*bo)[ret.numWrittenToBuf++];
              }
              uint16_t bpos = ret.numWrittenToBuf;
              EXPECT_EQ(PAYLOAD_SIZE, bpos);
              ASSERT_TRUE(dc.uncompressedNext(bo, bpos));

              EXPECT_EQ(68, ret.uncompressedSize);
              EXPECT_EQ((int)expSize, ret.compressedSize);
              ASSERT_FALSE(NULL == dc.getIPDatagram());
              ASSERT_EQ(1, datagram.getNumNextHeaders());
              ASSERT_EQ(cometos_v6::UDPPacket::HeaderNumber, datagram.getNextHeader()->getHeaderType());

              cometos_v6::UDPPacket* up = (cometos_v6::UDPPacket*)datagram.getNextHeader();

              EXPECT_EQ(udppacket.getSrcPort(), up->getSrcPort());
              EXPECT_EQ(udppacket.getDestPort(), up->getDestPort());
              EXPECT_EQ(datagram.getUpperLayerPayloadLength(), dc.getIPDatagram()->getUpperLayerPayloadLength());

              bo->free();
          }
      }

      datagram.removeHeader(0, false);
  }

}


#endif /* COMPRESSINGANDDECOMPRESSING_UNITTEST_H_ */
