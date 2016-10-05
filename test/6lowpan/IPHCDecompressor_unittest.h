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

#ifndef IPHCDecompressor_UNITTEST_H_
#define IPHCDecompressor_UNITTEST_H_

#include "gtest/gtest.h"
#include "IPHCDecompressor.h"

namespace {

class IPHCDecompressormTest : public ::testing::Test {
protected:
  IPv6Context srcCtx[16];
  IPv6Context dstCtx[16];
};

TEST_F(IPHCDecompressormTest, decompress) {
  cometos_v6::Ieee802154MacAddress srcMAC((uint16_t)1);
  cometos_v6::Ieee802154MacAddress dstMAC((uint16_t)2);

  const char* srcAddresses[2] = {"1::1", "FE80::25"};
  const char* dstAddresses[2] = {"1::2", "FE80::117"};

  int expectedSizes[2][2] = { 65, 57, 57, 49 };

  uint8_t numSrcAddresses = sizeof(srcAddresses)/sizeof(char*);
  uint8_t numDstAddresses = sizeof(dstAddresses)/sizeof(char*);

  cometos_v6::UDPPacket udppacket(50000, 50001);

  uint8_t udpSrcPortH = (uint8_t)(udppacket.getSrcPort() >> 8);
  uint8_t udpSrcPortL = (uint8_t) udppacket.getSrcPort();
  uint8_t udpDstPortH = (uint8_t)(udppacket.getDestPort() >> 8);
  uint8_t udpDstPortL = (uint8_t)udppacket.getDestPort();

  uint32_t fl = 0x12345;
  uint8_t tc = 0x12;

  uint8_t checkContent[2][2][65]= {
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

//      uint8_t checkContent[39] = {0x61, 0x00, tc, (uint8_t)(fl>>16), (uint8_t)(fl>>8), (uint8_t)(fl), cometos_v6::UDPPacket::HeaderNumber,
//                           0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
//                           0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};

  for (uint8_t i = 0; i < numSrcAddresses; i++) {
      for (uint8_t j = 0; j < numDstAddresses; j++) {
//          std::cout << "i=" << (int) i << "|j=" << (int) j << std::endl;
          IPv6Address srcIP(srcAddresses[i]);
          IPv6Address dstIP(dstAddresses[j]);

          cometos::Airframe frame;

          // first IPHC byte is retrieved elsewhere
          for (int8_t n = expectedSizes[i][j]-1; n > 0; n--) {
//              std::cout << "n=" << (int) n << std::endl;
              frame << checkContent[i][j][n];
          }

          cometos_v6::IPHCDecompressor dc(checkContent[i][j][0], frame, srcMAC, dstMAC, srcCtx, dstCtx);

          ASSERT_FALSE(NULL == dc.getIPDatagram());

          EXPECT_EQ(srcIP, dc.getIPDatagram()->src);
          EXPECT_EQ(dstIP, dc.getIPDatagram()->dst);
          EXPECT_EQ(fl, dc.getIPDatagram()->getFlowLabel());
          EXPECT_EQ(tc, dc.getIPDatagram()->getTrafficClass());
      }
  }

}


} //namespace

#endif /* IPHCDecompressor_UNITTEST_H_ */
