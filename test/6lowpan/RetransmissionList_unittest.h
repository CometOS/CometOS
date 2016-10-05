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

#ifndef RETRANSMISSIONLIST_UNITTEST_H_
#define RETRANSMISSIONLIST_UNITTEST_H_

#include "gtest/gtest.h"
#include "LowpanBuffer.h"
#include "IPv6Request.h"
#include "RetransmissionList.h"
#include "UDPPacket.h"
#include "DirectBuffer.h"
#include "RemoteModule.h"
#include "Module.h"

cometos_v6::BufferInformation* setDatagram(
        cometos_v6::IPv6RequestInformation& ipInformation,
        cometos_v6::IPv6Datagram& datagram,
        cometos_v6::UDPPacket& udppacket,
        uint16_t srcMac,
        uint16_t  dstMac);

class RegisterListTest : public ::testing::Test{
    public:
        cometos_v6::BufferInformation* createUDPDatagram(
                cometos_v6::IPv6Request& ipInformation,
                cometos_v6::IPv6Datagram& datagram,
                cometos_v6::UDPPacket& udppacket,
                cometos_v6::Ieee802154MacAddress& srcMac,
                cometos_v6::Ieee802154MacAddress&  dstMac){
            ipInformation.req.datagram = &datagram;
            ipInformation.req.dstMacAddress = dstMac;
            ipInformation.req.srcMacAddress = srcMac;

            datagram.addHeader(&udppacket);
            datagram.setHopLimit(1);
            datagram.setFlowLabel(0x12345);
            datagram.setTrafficClass(0x12);

            udppacket.setSrcPort(0xF012);
            udppacket.setDestPort(0xF034);

            cometos_v6::BufferInformation* payloadMemory =
                    buffer.getBuffer(20);
            udppacket.setData(payloadMemory->getContent(),
                    payloadMemory->getSize());
            udppacket.generateChecksum(datagram.src, datagram.dst);

/*            std::cout<< "HeaderSize:" << datagram.getHeaderSize()<< std::endl;
            std::cout<< "Size:" << datagram.getSize()<< std::endl;*/
            return payloadMemory;
        }
    protected:
        cometos_v6::LowpanBuffer<1800,4> buffer;
};




TEST_F(RegisterListTest, insertDatagramInformation){
    cometos_v6::IpRetransmissionList<1>list;
    cometos_v6::IPv6Request ipInformation;
    cometos_v6::IPv6Datagram datagram;
    cometos_v6::UDPPacket udppacket;
    uint16_t tag =1;
    cometos_v6::Ieee802154MacAddress srcMac((uint16_t) 1);
    cometos_v6::Ieee802154MacAddress dstMac_1((uint16_t) 2);
    datagram.src.tryParse("1::1");
    datagram.dst.tryParse("1::2");
    cometos_v6::BufferInformation*	payloadMemory =
            createUDPDatagram(ipInformation, datagram,
                    udppacket, srcMac, dstMac_1);

    EXPECT_TRUE(list.addDatagram(&ipInformation, payloadMemory, tag));
    EXPECT_FALSE(list.addDatagram(&ipInformation, payloadMemory, tag));

    datagram.removeHeader(0, false);
}


TEST_F(RegisterListTest, retriveDatagram){

    cometos_v6::IpRetransmissionList<3>list;
    cometos_v6::IPv6Request ipInformation, ipInformation_1;
    cometos_v6::IPv6Datagram datagram, datagram_1;
    cometos_v6::UDPPacket udppacket, udppacket_1;
    cometos_v6::Ieee802154MacAddress srcMac((uint16_t) 1);
    cometos_v6::Ieee802154MacAddress dstMac_1((uint16_t) 2);
    cometos_v6::Ieee802154MacAddress dstMac_2((uint16_t) 3);

    datagram.src.tryParse("1::1");
    datagram.dst.tryParse("1::2");
    datagram_1.src.tryParse("1::1");
    datagram_1.dst.tryParse("1::3");
    uint16_t tag1 = 0;
    uint16_t tag2 = 1;

    cometos_v6::BufferInformation*	payloadMemory =
            createUDPDatagram(ipInformation, datagram, udppacket, srcMac, dstMac_1);
    cometos_v6::BufferInformation* payloadMemory_1 =
            createUDPDatagram(ipInformation_1, datagram_1, udppacket_1,
                    srcMac, dstMac_2);

    list.addDatagram(&ipInformation, payloadMemory, tag1);
    list.addDatagram(&ipInformation_1, payloadMemory_1, tag2);

    cometos_v6::DatagramInformation* reterivedInfo =
            list.retriveDatagram(tag1, dstMac_1);
    EXPECT_EQ(payloadMemory, reterivedInfo->getPayLoadBuffer());
    EXPECT_EQ(tag1, reterivedInfo->getTag());
    EXPECT_EQ(&ipInformation, reterivedInfo->getipRequest());

    cometos_v6::DatagramInformation* reterivedInfo_1 =
            list.retriveDatagram(tag2, dstMac_2);
    EXPECT_EQ(payloadMemory_1, reterivedInfo_1->getPayLoadBuffer());
    EXPECT_EQ(tag2, reterivedInfo_1->getTag());
    EXPECT_EQ(&ipInformation_1, reterivedInfo_1->getipRequest());

    cometos_v6::DatagramInformation* reterivedInfo_2 =
            list.retriveDatagram(tag2, dstMac_1);
    EXPECT_EQ(reterivedInfo_2, NULL);

    // clean up datagram.. skip this and destructor
    // tries to free static memory and then error...
    datagram.removeHeader(0, false);
    datagram_1.removeHeader(0, false);
}

TEST_F(RegisterListTest, testTickExpiry){


    cometos_v6::IpRetransmissionList<1> list;
    //cometos_v6::IPv6RequestInformation ipInformation;
    cometos_v6::IPv6Request ipInformation;
    cometos_v6::IPv6Datagram datagram;
    cometos_v6::UDPPacket udppacket;
    uint16_t tag =1;
    cometos_v6::Ieee802154MacAddress srcMac((uint16_t) 1);
    cometos_v6::Ieee802154MacAddress dstMac_1((uint16_t) 2);

    cometos_v6::BufferInformation*	payloadMemory =
            createUDPDatagram(ipInformation, datagram,
                    udppacket, srcMac, dstMac_1);
    list.addDatagram(&ipInformation, payloadMemory, tag);

    list.tick();
    cometos_v6::DatagramInformation* reterivedInfo =
            list.retriveDatagram(tag, dstMac_1);
    EXPECT_TRUE(reterivedInfo != NULL);

    list.tick();
    reterivedInfo = list.retriveDatagram(tag, dstMac_1);
    EXPECT_TRUE(reterivedInfo == NULL);

    list.tick();
    reterivedInfo = list.retriveDatagram(tag, dstMac_1);
    EXPECT_TRUE(reterivedInfo == NULL);

    EXPECT_TRUE(list.addDatagram(&ipInformation, payloadMemory, tag));

    datagram.removeHeader(0, false);
}

#endif /* RETRANSMISSIONLIST_UNITTEST_H_ */
