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

/**
 * @author Patrick Fuhlert
 */

#include "BorderRouter.h"

using namespace cometos_v6;

// Define the MTU for one packet. Should be equal or greater than MTU of TUN Device.
const int MAXIMUM_TRANSMISSION_UNIT = 1500;
const int UDP_HEADER_SIZE = 8;
//const char* ADDRESSES_XML = "innerAddresses.xml";

/**
 * Sample IP Address objects.
 */
//const IPv6Address outerAddress = IPv6Address(0x2001, 0x0db8, 0x0, 0xf101, 0x0,
//        0x0, 0x0, 0x2); // TUNTAP addr.
//const IPv6Address localhost = IPv6Address(0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1);
//const IPv6Address em1_fe80 = IPv6Address(0xfe80,0x0,0x0,0x0,0x92b1,0x1cff,0xfe7a,0x1289);
//const IPv6Address innerAddress = IPv6Address(0x64, 0x29, 0x30, 0x31, 0x0, 0x0,
//        0x0, 0x81A4);
//const IPv6Address bsAddress = IPv6Address(0x64, 0x29, 0x30, 0x31, 0x0, 0x0, 0x0,
//        0x81A4);
//const IPv6Address routerAddress = IPv6Address(0x64, 0x29, 0x30, 0x31, 0x0, 0x0,
//        0x0, 0x0);

/**
 * Sample Packet that could be used by
 * @see printDatagram()
 */
//const uint8_t ipstrm[50] = {0x60, 0x10, 0x00, 0x00, 0x00, 0x0a, 0x11, 0x40,
//        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//        0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x88, 0x65, 0x2c, 0x07,
//        0x00, 0x0a, 0xd1, 0x79, 0x68, 0x69 };

/**
 *  Sample UDP/IPv4 Packet stream
 */
//    const uint8_t testipudpv4[39] = {0x45, 0x00, 0x00, 0x27, 0x00, 0x00, 0x40, 0x00,
//                                  0x40, 0x11,
//                                  0x93, 0xBA, // chksm ipheader
//                                  0x86, 0x1c, 0x4d, 0x6a, // src ip
//                                  0x86, 0x1c, 0x4d, 0x69, // dst ip
//                                  0x82, 0xb0, // src port
//                                  0x56, 0xce, // dst port
//                                  0x00, 0x13,
//                                  0xad, 0xc4, // chksm udp
//                                  0x48, 0x41, 0x4c, 0x4c,
//                                  0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x4c, 0x4f};

/**
 * Outputs IP Header+Data (UDP Header too if possible) to cout
 *
 * @param dg Datagram to print
 *
 */
void printDatagram(cometos_v6::IPv6Datagram *dg) {
    std::cout << std::endl << "IP Header contains: " << std::endl;
    std::cout << "\tVersion: \t\t6" << std::endl;
    std::cout << "\tTraffic Class: \t\t" << (int) dg->getTrafficClass()
            << std::endl;
    std::cout << "\tFlow Label: \t\t" << dg->getFlowLabel() << std::endl;
    std::cout << "\tPayload Length: \t" << dg->getSize() << " Byte"
            << std::endl;

    std::cout << "\tHop Limit: \t\t" << std::dec << (int) dg->getHopLimit() << std::endl;
    std::cout << "\tSource Address: \t" << dg->src.str() << std::endl;
    std::cout << "\tDestination Address: \t" << dg->dst.str() << std::endl;

    //std::cout << "\tHeader Data: \t" << dataToString(dg->getNextHeader()->getData(),dg->getNextHeader()->getDataLength()) << std::endl;

    std::cout << "\tFollowing Header: \t" << (int) dg->getNextHeader()->getHeaderType()
            << std::endl;

    if(dg->getLastHeader()->getHeaderType() == UDPPacket::HeaderNumber) {
        UDPPacket* nh = (UDPPacket*) dg->getLastHeader();

        std::cout << "UDP Header contains: " << std::endl;
        std::cout << "\tSRC Port: " << nh->getSrcPort() << std::endl;
        std::cout << "\tDST Port: " << nh->getDestPort() << std::endl;
        std::cout << "\tLength: " << nh->getDataLength() << std::endl;
/*
        uint8_t buf[MAXIMUM_TRANSMISSION_UNIT];

        memcpy(buf,nh->getData(),nh->getDataLength());
        buf[nh->getDataLength()] = '\0';

        std::cout << "\tData: " << buf << std::endl;
*/
    }

    std::cout << std::dec << std::endl;
}

/**
 * Creates formatted string representation of data stream hex values
 *
 * @param data bytestream
 * @param length length of bytestream
 *
 * @return formatted string
 */
static string streamToString(const uint8_t* data, uint16_t length) {
    std::stringstream os;
    for (int i = 0; i < length; i++) {
        if (i % 8 == 0) {
            os << endl << std::dec << i << ":\t";
        }

        os << std::hex << std::setfill('0') << std::setw(2) << (int) data[i]
                << "\t";
    }
    os << endl;
    return os.str();
}

/**
 * Thread to listen to incoming packets of the extern world (non 6LoWPAN) via
 * TUN Device. 'read' is blocking so an extra thread is needed for that.
 *
 * @param h Thread Handler
 * @param p Pointer to calling object
 */
static void listenerThread(thread_handler_t h, void* p) {

    // Get reference to BorderRouter instance
    BorderRouter* br = (BorderRouter*) (p);

    while (!thread_receivedStopSignal(h)) {

        uint8_t buffer[MAXIMUM_TRANSMISSION_UNIT]; // MTU TODO größer?
        int nread;
        /* Note that "buffer" should be at least the MTU */
        // "Each read() returns a full packet"
        nread = read(br->tt->getFd(),buffer,sizeof(buffer));

        if(nread < 0) {
            cout << "listenerThread(real)|Error reading from interface";
            close(br->tt->getFd());
            exit(1);
        }

//        cout << "listenerThread(real)|Got new package with " << nread << " bytes." << endl;


        // Get Datagram from stream - myparse vs cometparse

//        cout << streamToString(buffer, nread);

        IPv6Datagram* newUDPIPDatagram = br->parseUDPIPv6(buffer,nread);

//        IPv6Datagram* newUDPIPDatagram = new IPv6Datagram();
//        newUDPIPDatagram->parse(buffer,nread);

        // Process the packet (send to inner Network if IP matches or discard it.
        if (newUDPIPDatagram != NULL) {
            br->fromExtern(newUDPIPDatagram);
        }
    }

    // When the Stop signal for the thread was received, close it.
    thread_close(h);
    return;
}

/**
 * Constructor of BorderRouter class. Create as module and gate connections. to use with OmNET++
 */
BorderRouter::BorderRouter(const char* ip_prefix) :
        cometos::Module("br"), toLowpan(this, "toLowpan"), fromLowpan(this,
                &BorderRouter::handleRequestFromLowpan, NULL),
                toIPv4(false),
                ip_prefix(ip_prefix) {
}

/**
 * Destructor of BorderRouter class.
 */
BorderRouter::~BorderRouter() {
    thread_destroyMutex(mutex);
    // send end thread signal
}

/**
 * Initializing. Creates mutex, TUNTAP device and sets the addresses of 6LoWPAN network. Finally it starts the listenerThread.
 */
void BorderRouter::initialize() {

    mutex = thread_createMutex();
    tt = new TunTap("tunBR", ip_prefix);

//    BorderRouter::setInnerAddresses();

    thread_lockMutex(mutex);
    thread_run(listenerThread, this);
    thread_unlockMutex(mutex);
}

/**
 * Parses UDP/IPv6 Packet. Should be implemented in IPv6Datagram
 * Format of Bytestream:
 *
 * IPv6 (40 Bytes) | UDP (8 Bytes) | Data
 *
 * @param buffer Bytestream representing the packet
 * @param length length of Bytestream
 *
 */

cometos_v6::IPv6Datagram* BorderRouter::parseUDPIPv6(const uint8_t* buffer, uint16_t length) {
    IPv6Datagram* tempPkg = new IPv6Datagram();

    if ((buffer[0] & 0xF0) == 0x60) {
        uint16_t i = tempPkg->parse(buffer, length);
        if (i == length) {
            return tempPkg;
        }
        LOG_WARN("Error during Parsing " << i << " " << length << streamToString(buffer, length));
    } else if (toIPv4 && ((buffer[0] & 0xF0) == 0x40) && (buffer[16] == 192) && (buffer[17] == 168)) {
        LOG_INFO("Received IPv4 Packet");

        if (buffer[9] == UDPPacket::HeaderNumber) {

            tempPkg->src = "::FFFF:0:0:0";
            tempPkg->dst = ip_prefix;

            tempPkg->src.setAddressPart(buffer[12], buffer[13], 6);
            tempPkg->src.setAddressPart(buffer[14], buffer[15], 7);

            tempPkg->dst.setAddressPart(buffer[18], buffer[19], 7);

            tempPkg->setHopLimit(buffer[8]);

            UDPPacket* udpPkt = new UDPPacket(0, 0);
            udpPkt->parse(&buffer[20], length - 20);
            udpPkt->generateChecksum(tempPkg->src,  tempPkg->dst);

            tempPkg->addHeader(udpPkt, -1);

            return tempPkg;
        }
        LOG_WARN("Unrecognized Next Header " << (int)(buffer[9]));
    }
    delete tempPkg;
    return NULL;

//    uint16_t ret = 0;
//    if (length > 39) {
//        uint8_t trafficClass = (buffer[ret] << 4) |
//                (buffer[ret + 1] >> 4);
//        ret++;
//        uint32_t flowLabel  = (((uint32_t) buffer[ret] & 0x0F) << 16)
//                | ((uint16_t) buffer[ret + 1] << 8) | buffer[ret + 2];
//        ret += 3;
//        // uint16_t pLen = (buffer[ret] << 8) | (buffer[ret+1]);
//
//        ret += 2;
//
//        FollowingHeader* nxt = tempPkg->createNextHeader(buffer[ret]);
//        tempPkg->setNextHeader(nxt);
//
//        ret++;
//        uint8_t hopLimit = (buffer[ret]);
//        ret++;
//        tempPkg->src = &(buffer[ret]);
//        ret += 16;
//        tempPkg->dst = &(buffer[ret]);
//        ret += 16;
//
//        tempPkg->setTrafficClass(trafficClass);
//        tempPkg->setFlowLabel(flowLabel);
//        tempPkg->setHopLimit(hopLimit);
//
//        if (ret < length) {
//            if (buffer[6] == UDPPacket::HeaderNumber) { // IP identifier for next header (UDP=17) on pos 6
//                UDPPacket* tempUDP = new UDPPacket(0, 0);
//                tempUDP->parse(&buffer[40], length - ret); // TODO anders? // buffer[40] is first of new header
//                tempPkg->addHeader(tempUDP, -1);
//            }
//
//        }
//    }
//    return tempPkg;
}

/**
 * See if given address object is a recognized address of 6LoWPAN network defined by @see BorderRouter:setInnerAddresses
 *
 * @param addr address object
 *
 * @return True if address is member of 6LoWPAN network
 */
bool BorderRouter::isInnerAddress(IPv6Address addr) {
//    std::list<IPv6Address>::iterator findIter = std::find(
//            innerAddresses.begin(), innerAddresses.end(), addr);
//    return (findIter != innerAddresses.end());
    return (addr.matches(ip_prefix, 112));
}

/**
 * Define the addresses of 6LoWPAN nodes and add them to a list so they can be recognized.
 *
 * ADDRESSES_XML file needed
 */
//void BorderRouter::setInnerAddresses() {
//
//    // Get Addresses xml
//    pugi::xml_document doc;
//    doc.load_file(ADDRESSES_XML);
//
//    for (pugi::xml_node node = doc.child("node"); node != NULL;
//
//            node = node.next_sibling()) {
//
//        // Get String value of address by iterator and let IPv6Address(const char* txt) constructor parse IPv6Address object.
//        IPv6Address* newIP = new IPv6Address(
//                node.child("address").text().get());
//
////         LOG_INFO("Parsing OK? Address to add: " << newIP->str());
//
//        // Add to Borderrouter's list of inner nodes
//        innerAddresses.push_back(*newIP);
//    }
//
//// Alternative: ASSERT(!innerAddresses.empty())
//
//    if (innerAddresses.empty()) {
//        LOG_ERROR("No Nodes Added! The Borderrouter will not provide any packets to 6LoWPAN nodes. Use 'innerAddresses.xml' to set nodes!"); // todo if length=0
//    } else {
//        // Output 6LoWPAN nodes
//        LOG_INFO("Created Whitelist for 6LoWPAN network with " << innerAddresses.size() << " Addresses:");
//        for (std::list<IPv6Address>::iterator iterator = innerAddresses.begin();
//                iterator != innerAddresses.end(); ++iterator) {
//            LOG_INFO(iterator->str());
//        }
//    }
//}

/**
 * Method that listens for 6LoWPAN responses.
 *
 * @param response The response to corresponding request.
 */
void BorderRouter::handleResponseFromLowpan(IPv6Response * response) {

    if (response->success == IPv6Response::IPV6_RC_SUCCESS) {
        LOG_INFO("Response Success!");
    } else {
        LOG_INFO("Response Failed!");
        return; // TODO what?
    }

    IPv6Request * originalReq = response->refersTo;
    ASSERT(originalReq != NULL);
    ASSERT(originalReq->datagram != NULL);

    /* LOG orig. REQ */
    //    LOG_INFO("SRC:" << originalReq->datagram->src.str());
    //    LOG_INFO("DST:" << originalReq->datagram->dst.str());
    //    LOG_INFO("MACSRC: " << std::hex << originalReq->srcMacAddress.a4());
    //    LOG_INFO("MACDST: " << std::hex << originalReq->dstMacAddress.a4());
    //    LOG_INFO("HOPLMT: " << (int) originalReq->datagram->getHopLimit());

    // done
    delete response;
    delete originalReq;
}

/**
 * Method that listens for 6LoWPAN requests. Send to extern world if packet destination says so.
 *
 * @param iprequest Pointer to corresponding request.
 */
void BorderRouter::handleRequestFromLowpan(cometos_v6::IPv6Request *iprequest) {

//    LOG_INFO("IPSRC:" << iprequest->datagram->src.str());
//    LOG_INFO("IPDST:" << iprequest->datagram->dst.str());
//    LOG_INFO("MACSRC: " << std::hex << iprequest->srcMacAddress.a4());
//    LOG_INFO("MACDST: " << std::hex << iprequest->dstMacAddress.a4());

    printDatagram(iprequest->datagram);

    // TODO let this be a normal hop via router to other 6LoWPAN node?
    if(isInnerAddress(iprequest->datagram->dst) && iprequest->datagram->dst != ip_prefix) {
        LOG_ERROR("This Packet should not be here, drop.");
        iprequest->response(new cometos_v6::IPv6Response(iprequest, false));
        return;
    }

    iprequest->datagram->decrHopLimit();

    toExtern(iprequest->datagram);

    // Success
    iprequest->response(new cometos_v6::IPv6Response(iprequest, true));
}

/**
 * Processes incoming Packets from TUN Device
 *
 * @param dg corresponding (parsed) datagram.
 *
 */
void BorderRouter::fromExtern(cometos_v6::IPv6Datagram* dg) { // TODO in borderrouter integrieren, unthread

    if (dg != NULL) {
        printDatagram(dg);
    }

    if (dg != NULL && isInnerAddress(dg->dst)) { // Address is on Whitelist!

        LOG_INFO("Send PKG with DST " << dg->dst.str() << " to 6LoWPAN.");

        // Wait for Response coming to @see BorderRouter:handleResponseFromLoWPAN
        IPv6Request * req = new IPv6Request(createCallback(&BorderRouter::handleResponseFromLowpan));
        req->datagram = dg;

        // Convention: Last 4 hex values of node represent MAC and IP Address.
        Ieee802154MacAddress src((uint16_t) 0x0); // Router MAC
        Ieee802154MacAddress dst((uint16_t) dg->dst.getAddressPart(7));

        req->dstMacAddress = dst;
        req->srcMacAddress = src;

        toLowpan.send(req);

    } else if (dg != NULL) {
        // TODO host not reachable reply? (ICMP)
        LOG_INFO("PKG with DST " << dg->dst.str() << " from " << dg->src.str() << " deleted. (not on Whitelist!)");
        delete dg;
    } else {
        LOG_WARN("Unparsable Message");
    }
}

/**
 * Send desired datagram to extern world.
 *
 * @param dg datagram to send
 */
void BorderRouter::toExtern(cometos_v6::IPv6Datagram* dg) {

    //pkg from inner network detected? something must have gone wrong
    ASSERT(!isInnerAddress(dg->dst) || dg->dst == ip_prefix);

    //LOG_INFO("Sending stream to extern.");

    if ((toIPv4 && dg->dst.matches(IPv6Address(0,0,0,0,0xFFFF,0,0,0), 96)) &&
            dg->getLastHeader()->getHeaderType() == UDPPacket::HeaderNumber) {

        uint8_t buffer[MAXIMUM_TRANSMISSION_UNIT];
        uint8_t bufferHeader[MAXIMUM_TRANSMISSION_UNIT];
        uint8_t bufferData[MAXIMUM_TRANSMISSION_UNIT];

        /*
         * Due to missing IPv6 network it has to be 'converted' to use it in current IPv4 network.
         */

        // Get UDP subbytestream
        UDPPacket* nh = (UDPPacket*) dg->getLastHeader();
    //    nh->generateChecksum(dg->src, dg->dst); // Only for IPv6, wont work here!
        nh->setChecksum(0x0); // dont use because of error merging to IPv4
        uint16_t headerSize = nh->writeHeaderToBuffer(bufferHeader);
        uint16_t dataSize = nh->getDataLength();
        memcpy(bufferData, nh->getData(),dataSize); // TODO mit IPv4 maximal 2^8

        // HACK ugly: use static IPv4 header with static addresses, manual checksum adaption required when data size of udp packet changes
        const uint16_t IPV4UDPSIZE = 20;

        uint16_t packetSize = IPV4UDPSIZE+headerSize+dataSize;

        uint8_t srcIP[4];

        srcIP[0] = 192;
        srcIP[1] = 168;
        srcIP[2] = dg->src.getAddressPart(7) >> 8;
        srcIP[3] = dg->src.getAddressPart(7) & 0xFF;

        uint8_t dstIP[4] = {192, 168, 0, 0};

        if (dg->dst.matches(IPv6Address(0,0,0,0,0xFFFF,0,0,0), 96)) {
            dstIP[0] = dg->dst.getAddressPart(6) >> 8;
            dstIP[1] = dg->dst.getAddressPart(6) & 0xFF;
            dstIP[2] = dg->dst.getAddressPart(7) >> 8;
            dstIP[3] = dg->dst.getAddressPart(7) & 0xFF;
        }

        uint8_t ttl = dg->getHopLimit();

        uint8_t IPv4UDP[IPV4UDPSIZE] = {
                0x45, 0x00,     // Version and Headersize
                (uint8_t)(packetSize >> 8), (uint8_t)packetSize,     // total size
                0x00, 0x00,     // Identification
                0x40, 0x00,     // Flags and Fragment Offset
                ttl,            // TTL
                0x11,           // Protocol
                0, 0,           // Chksum comes later
//                (uint8_t)(ipChkSm >> 8), (uint8_t)ipChkSm,  // chksm ipheader
                srcIP[0], srcIP[1], srcIP[2], srcIP[3],     // src ip 192.168.x.x
                dstIP[0], dstIP[1], dstIP[2], dstIP[3]};    // dst ip 192.168.0.0

        uint32_t ipChkSm = 0;
        for (uint8_t i = 0; i < IPV4UDPSIZE; i+=2) {
            ipChkSm += (((uint16_t)(IPv4UDP[i]))<<8) | IPv4UDP[i+1];
        }

        ipChkSm = (ipChkSm & 0xFFFF) + (ipChkSm >> 16);
        ipChkSm = ~ipChkSm;

        IPv4UDP[10] = (uint8_t)(ipChkSm >> 8);
        IPv4UDP[11] = (uint8_t)ipChkSm;

        if(packetSize > MAXIMUM_TRANSMISSION_UNIT) {
            // ERROR. Max Size too big. Drop packet.
            return;
        }

        // merge streams to form output packet bytestream
        memcpy(buffer, IPv4UDP,IPV4UDPSIZE);
        memcpy(buffer+IPV4UDPSIZE, bufferHeader,headerSize);
        memcpy(buffer+IPV4UDPSIZE+headerSize, bufferData,dataSize);

        tt->send(buffer, packetSize);
        LOG_INFO("Sent IPv4 PKG stream to extern network. L: " << dataSize);
    } else {
        uint16_t size = dg->getSize();
        if (MAXIMUM_TRANSMISSION_UNIT >= size) {
            uint8_t buffer[MAXIMUM_TRANSMISSION_UNIT];

            uint16_t pos = dg->writeHeaderToBuffer(buffer);

            if (pos != 0) {

                FollowingHeader* last = dg->getLastHeader();

                switch(last->getHeaderType()) {
                case (UDPPacket::HeaderNumber):
                    memcpy(&(buffer[pos]), ((UDPPacket*)last)->getData(), ((UDPPacket*)last)->getDataLength());
                    break;
                case (ICMPv6Message::HeaderNumber):
                    memcpy(&(buffer[pos]), ((ICMPv6Message*)last)->getData(), ((ICMPv6Message*)last)->getDataLength());
                    break;
                default:
                    LOG_WARN("Unknown Header hn:" << (int)last->getHeaderType());
                    return;
                }

                tt->send(buffer, size);
                LOG_INFO("Sent IPv6 PKG stream to extern network. L: " << size);
            } else {
                LOG_WARN("Error writing header");
            }

        } else {
            LOG_WARN("MTU Exceeded");
        }
    }
    return;
}

/*** Serialize of 6in4 test packet ***/
//
//    UDPPacket* nh = (UDPPacket*) dg->getNextHeader();
//    nh->generateChecksum(dg->src, dg->dst); // falsches ergebnis!
//    nh->setChecksum(0xd6ae); // fake chksm
//
//    int headerSize = dg->writeHeaderToBuffer(bufferHeader);
//
//    int dataSize = nh->writeHeaderToBuffer(bufferHeader);
//    memcpy(bufferData, nh->getData(),dataSize);
//
//    // LOG_INFO("Data: " << streamToString(bufferData,nh->getDataLength()));
//
//
//    // 6in4 Tunneling
//    const uint16_t TUNL6IN4SIZE = 20;
//    const uint8_t tunl6in4[TUNL6IN4SIZE] = {0x45, 0x00, 0x00, 0x4A, 0x00, 0x00, 0x40, 0x00,
//                              0x40,
//                              0x29, // Protokoll ipv6 following
//                              0x93, 0x7f, // chksm ipheader
//                              0x86, 0x1c, 0x4d, 0x6a, // src ip 134.28.77.106
//                              0x86, 0x1c, 0x4d, 0x69}; // dst ip 134.28.77.105
//
//
//    // merge
//    memcpy(buffer, tunl6in4,TUNL6IN4SIZE);
//    memcpy(buffer+20, bufferHeader,headerSize);
//    memcpy(buffer+20+headerSize, bufferData,dataSize);
//    // LOG_INFO("Merged: " << streamToString(buffer,TUNL6IN4SIZE+headerSize+dataSize));
//    tt->send(buffer, TUNL6IN4SIZE+headerSize+dataSize);
/*** end of serialize ***/
