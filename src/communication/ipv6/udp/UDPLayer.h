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
 * @author Martin Ringwelski
 */

#ifndef UDPLAYER_H_
#define UDPLAYER_H_

#include "cometos.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "PersistableConfig.h"
#include "RemotelyConfigurableModule.h"

#define UDP_SCALAR(x)           uint16_t x
#define UDP_SCALAR_INI(x, y)    x(y)
#define UDP_SCALAR_INC(x)       stats.x++


#define UDP_MODULE_NAME     "udp"

namespace cometos_v6 {

const uint8_t UDP_MAX_LISTENERS =       5;
const uint8_t UDP_MAX_CONTENTREQUESTS = 8;

struct UdpLayerStats {
    UdpLayerStats() :
        UDP_SCALAR_INI(numSent, 0),
        UDP_SCALAR_INI(numNotSent, 0),
        UDP_SCALAR_INI(numReq, 0),
        UDP_SCALAR_INI(numOutOfReqs, 0)
    {}

    void reset() {
        numSent = 0;
        numNotSent = 0;
        numReq = 0;
        numOutOfReqs = 0;
    }

    UDP_SCALAR(numSent);
    UDP_SCALAR(numNotSent);
    UDP_SCALAR(numReq);
    UDP_SCALAR(numOutOfReqs);
};

struct UdpConfig : public cometos::PersistableConfig {
    UdpConfig(uint8_t numContentRequests = UDP_MAX_CONTENTREQUESTS) :
        numContentRequests(numContentRequests)
    {}

    virtual void doSerialize(cometos::ByteVector& buf) const;
    virtual void doUnserialize(cometos::ByteVector& buf);

    bool operator==(const UdpConfig& rhs) {
        return this->numContentRequests == rhs.numContentRequests;
    }

    bool isValid() {
        return numContentRequests < 40;
    }

    uint8_t numContentRequests;
};

class UDPListener {
public:
    virtual void udpPacketReceived(const IPv6Address& src,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length) = 0;

    virtual ~UDPListener() {};
};

class UDPLayer : public cometos::RemotelyConfigurableModule<UdpConfig> {
public:
#if defined SWIG || defined BOARD_python
    enum status_t
#else
    enum status_t : uint8_t
#endif
    {
        AWAIT_SENDING =   0xFF,
        SUCCESS =         0x00,
        FAILURE =         0x0F
    };

    UDPLayer(const char * service_name = NULL);
    ~UDPLayer();
    void initialize();

    void finish();

    void handleIPRequest(ContentRequest *cRequest);

    void handleIPResponse(ContentResponse *cResponse);

    uint16_t bind(UDPListener * listener, uint16_t port);

    bool unbindPort(uint16_t port);

    bool registerSniffer(UDPListener * listener);
    bool deregisterSniffer(UDPListener * listener);

    /**
     * Sends a message to the given destination and port.
     * For efficient memory handling, the UDPLayer can deal with a data pointer
     * that points into the LOWPAN buffer and refrains from copying the data if
     * such a pointer is given. In that case, the caller yields ownership of the
     * passed pointer to the UDP (and below) layers. If a pointer is given that
     * does not point to the LOWPAN buffer (obtained by using
     * LowpanAdaptionLayer::getLowpanDataBuffer()), the UDP layer will copy the
     * data into it and the ownership of the memory is passed back to the
     * caller when the function returns.
     *
     * @param[in] dst     IPv6 address of the target of this message
     * @param[in] srcPort port to be used at this node (origin of message)
     * @param[in] dstPort destination port
     * @param[in] data    pointer to data to be sent. If pointing to the lowpan
     *                    buffer, memory ownership is yielded by the caller; if
     *                    not, ownership is returned when method returns
     * @param[in] length  size of data buffer and message to be sent

     * @param[in,out] statusByte
     *   can be optionally used to signal the state of the UDP transmission
     *   back to the caller, in case it is required. Leave NULL if no
     *   such signal is desired.
     *
     */
    bool sendMessage(const IPv6Address& dst,
            uint16_t srcPort,
            uint16_t dstPort,
            const uint8_t* data,
            uint16_t length,
            status_t* statusByte);

    bool sendMessage(const IPv6Address& dst,
            uint16_t srcPort,
            uint16_t dstPort,
            BufferInformation* bi,
            status_t* statusByte);

    uint16_t getFreePort(uint16_t port = 0);

    virtual bool isBusy();
    virtual void applyConfig(UdpConfig& cfg);
    virtual UdpConfig& getActive();

    UdpLayerStats getStats() {
        return stats;
    }

    void resetStats() {
        stats.reset();
    }


    cometos::InputGate<ContentRequest>     fromIP;
    cometos::OutputGate<ContentRequest>    toIP;

private:


    uint8_t getNextFreeListener();

    ContentRequest* getContentRequest(BufferInformation* data = NULL, status_t* statusByte = NULL);
    void freeContentRequest(ContentRequest* cr, status_t status);

    struct ListenerHandler {
        ListenerHandler() :
            port(0), listener(NULL)
        {}
        bool operator==(const ListenerHandler & other) const {
            return this->listener == other.listener && this->port == other.port;
        }

        uint16_t         port;
        UDPListener *    listener;
    } listeners[UDP_MAX_LISTENERS];

    void deleteMessagePool();

    void createMessagePool(const UdpConfig& cfg);

    UdpConfig cfg;

    contentRequestHolder_t* contentRequests;
    status_t**              statusBytes;
//    contentRequestHolder_t  contentRequests[UDP_MAX_CONTENTREQUESTS];
//    status_t*               statusBytes[UDP_MAX_CONTENTREQUESTS];

    UdpLayerStats stats;
};

}

namespace cometos {
void serialize(ByteVector & buf, const cometos_v6::UdpLayerStats & value);
void unserialize(ByteVector & buf, cometos_v6::UdpLayerStats & value);
}

#endif /* UDPLAYER_H_ */
