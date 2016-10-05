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

IPv6Request* BorderRouter::getTestICMP(cometos::Message * msg) {

    IPv6Address ipsrc(0x64, 0x29, 0x30, 0x31, 0x0, 0x0, 0x0,(uint16_t) palId_id());
    IPv6Address ipdst(0x64, 0x29, 0x30, 0x31, 0x0, 0x0, 0x0, 0x221A);

    Ieee802154MacAddress src((uint16_t) 0x0);
    Ieee802154MacAddress dst((uint16_t) 0x221A); // 0x2219
    IPv6Datagram * dg = new IPv6Datagram();
    IPv6Request * req = new IPv6Request(
            createCallback(&BorderRouter::handleResponseFromLoWPAN));
    req->datagram = dg;
    req->datagram->dst = ipdst;
    req->datagram->src = ipsrc;
    req->dstMacAddress = dst;
    req->srcMacAddress = src;
    //
    //     UDPPacket * udppacket = new UDPPacket(12345,10695);
    //    req->datagram->addHeader(udppacket);
    //
    ICMPv6Message* icmpmsg = new ICMPv6Message(128, 0);
    const uint8_t msg0[] = "";
    icmpmsg->setData(msg0, 1);
    icmpmsg->setChecksum(0x1234);
    //TODO always 0?
    icmpmsg->generateChecksum(ipsrc, ipdst);
    req->datagram->addHeader(icmpmsg);

    req->datagram->setTrafficClass(0x12);
    req->datagram->setFlowLabel(0x10);

    //LOG_DEBUG("ICMP OK? " << icmp->checkValid(ipsrc, ipdst));

    req->datagram->setHopLimit(128);

    //schedule(&timer, &BorderRouter::timerFired, 0);

    return req;

}
