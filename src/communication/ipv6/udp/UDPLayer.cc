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

#include "UDPLayer.h"
#include "UDPPacket.h"
#include "palLed.h"
#include "palId.h"
#include "LowpanAdaptionLayer.h"
namespace cometos_v6 {

Define_Module(UDPLayer);

UDPLayer::UDPLayer(const char * service_name) :
                cometos::RemotelyConfigurableModule<UdpConfig>(service_name),
                fromIP(this, &UDPLayer::handleIPRequest, "fromIP"),
                toIP(this, "toIP"),
                contentRequests(nullptr),
                statusBytes(nullptr)
{
}

UDPLayer::~UDPLayer() {
    deleteMessagePool();
}

void UDPLayer::deleteMessagePool() {
    if (contentRequests != nullptr) {
        for (uint8_t i = 0; i < cfg.numContentRequests; i++) {
            TAKE_MESSAGE(&(contentRequests[i].contentRequest));
            delete contentRequests[i].contentRequest.content;
        }
    }
    delete[] contentRequests;
    contentRequests = nullptr;
    delete[] statusBytes;
    statusBytes = nullptr;
}

void UDPLayer::createMessagePool(const UdpConfig& cfg) {
    ASSERT(contentRequests == nullptr);
    ASSERT(statusBytes == nullptr);
    contentRequests = new contentRequestHolder_t[cfg.numContentRequests];
    statusBytes = new status_t*[cfg.numContentRequests];
    for (uint8_t i = 0; i < cfg.numContentRequests; i++) {
        contentRequests[i].contentRequest.content =
                (FollowingHeader*)(new UDPPacket());
        contentRequests[i].contentRequest.setResponseDelegate(
                createCallback(&UDPLayer::handleIPResponse));
        statusBytes[i] = nullptr;
    }
}

void UDPLayer::initialize()
{
    RemotelyConfigurableModule<UdpConfig>::initialize();
    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
    if (ps != nullptr) {
        ps->getCfgData(this, cfg);
    }

    createMessagePool(cfg);
}

bool UDPLayer::isBusy() {
    // first make sure that no messages are flying around somewhere
    for (uint8_t i = 0; i < cfg.numContentRequests; i++) {
        if (contentRequests[i].occupied) {
            LOG_ERROR("Found occupied at " << (int) i);
            return true;
        }
    }
    return false;
}

void UDPLayer::applyConfig(UdpConfig& newCfg) {
    cfg = newCfg;

    deleteMessagePool();
    createMessagePool(newCfg);
}

UdpConfig& UDPLayer::getActive() {
    return cfg;
}


void UDPLayer::handleIPRequest(ContentRequest *cRequest)
{
    UDPPacket* udp = static_cast<UDPPacket*>(cRequest->content);
    bool success = false;
    if (udp->checkValid(cRequest->src, cRequest->dst)) {
        LOG_DEBUG("UDP<-IP:[" << cRequest->src.getAddressPart(7)
                << "]:" << udp->getSrcPort() << "->["
                << cRequest->dst.getAddressPart(7) << "]:" << udp->getDestPort());
        for (uint8_t i = 0; i < UDP_MAX_LISTENERS; i++) {
            //LOG_DEBUG("check if " << listeners[i].port << " == " << udp->getDestPort());
            if ((listeners[i].port == udp->getDestPort() || listeners[i].port == 0) &&
                    listeners[i].listener != NULL) {
                LOG_DEBUG("Calling Listening Module");
                listeners[i].listener->udpPacketReceived(
                        cRequest->src,
                        udp->getSrcPort(),
                        udp->getDestPort(),
                        udp->getData(),
                        udp->getUpperLayerPayloadLength());
                success = true;
            }
        }
        if (!success) {
            LOG_INFO("No Handler");
        }
    } else {
        LOG_WARN("UDP Pckt invalid");
    }
    cRequest->response(new ContentResponse(cRequest, success));
}

void UDPLayer::handleIPResponse(ContentResponse *cResponse)
{
    status_t status = SUCCESS;
    if (cResponse->success) {
        LOG_INFO("UDP succ");
        UDP_SCALAR_INC(numSent);
    } else {
        status = FAILURE;
        LOG_WARN("UDP unsucc");
        UDP_SCALAR_INC(numNotSent);
    }
    freeContentRequest(cResponse->refersTo, status);
    delete cResponse;
}

uint16_t UDPLayer::bind(UDPListener * listener, uint16_t port) {
    if (listener == NULL) return 0;
    port = getFreePort(port);
    if (port > 0) {
        uint8_t i = getNextFreeListener();
        if (i < UDP_MAX_LISTENERS) {
            LOG_DEBUG("Set Listener to Port " << port);
            listeners[i].listener = listener;
            listeners[i].port = port;
        } else {
            port = 0;
        }
    }
    return port;
}

bool UDPLayer::registerSniffer(UDPListener * listener) {
    uint8_t i = getNextFreeListener();
    if (i < UDP_MAX_LISTENERS) {
        listeners[i].listener = listener;
        listeners[i].port = 0;
        return true;
    }
    return false;
}

bool UDPLayer::deregisterSniffer(UDPListener * listener) {
    uint8_t i;

    for (i = 0;
            i < UDP_MAX_LISTENERS && listeners[i].listener != listener;
            i++);
    if (i < UDP_MAX_LISTENERS) {
        listeners[i].port = 0;
        listeners[i].listener = NULL;
        return true;
    }
    return false;
}


uint8_t UDPLayer::getNextFreeListener() {
    uint8_t i;
    for (i = 0;
            i < UDP_MAX_LISTENERS && listeners[i].listener != NULL;
            i++)
    {
        // just to get the correct index
    }
    LOG_DEBUG("i = " << (uint16_t)i);
    return i;
}

uint16_t UDPLayer::getFreePort(uint16_t port) {
    uint8_t i;
    if (port != 0) {
        for (i = 0; i < UDP_MAX_LISTENERS; i++) {
            if (listeners[i].port == port) {
                return 0;
            }
        }
    } else {
        port = 0xF0B0;
        bool free = false;
        while (!free) {
            free = true;
            for (i = 0; i < UDP_MAX_LISTENERS; i++) {
                if (listeners[i].port == port) {
                    free = false;
                    port++;
                    if (port == 0) port++;
                    if (port == 0xF0B0) return 0;
                }
            }
        }
    }
    return port;
}

bool UDPLayer::unbindPort(uint16_t port)
{
    uint8_t i;

    if (port != 0) {

        for (i = 0;
                i < UDP_MAX_LISTENERS && listeners[i].port != port;
                i++);
        if (i < UDP_MAX_LISTENERS) {
            listeners[i].port = 0;
            if (listeners[i].listener != NULL) {
                LOG_DEBUG("Unbind Port " << port);
                listeners[i].listener = NULL;
            }
            return true;
        }
    }
    return false;
}

bool UDPLayer::sendMessage(const IPv6Address& dst,
        uint16_t srcPort,
        uint16_t dstPort,
        const uint8_t* data,
        uint16_t length,
        status_t* statusByte)
{
    ENTER_METHOD_SILENT();
    if (statusByte != nullptr) *statusByte = AWAIT_SENDING;
    if (data != NULL && length > 0) {
        LowpanAdaptionLayer * lowpan = (cometos_v6::LowpanAdaptionLayer *) getModule(LOWPAN_MODULE_NAME);

        BufferInformation* bi = lowpan->getContainingLowpanDataBuffer(data);

        if (bi == NULL) {
            bi = lowpan->getLowpanDataBuffer(length);
            if (bi == NULL) {
                if (statusByte != nullptr) *statusByte = FAILURE;
                LOG_INFO("Buffer Full");
                return false;
            }
            bi->copyToBuffer(data, length);
        }

        return sendMessage(dst, srcPort, dstPort, bi, statusByte);
    } else {
        ASSERT(false);
        return false;
    }
}

bool UDPLayer::sendMessage(const IPv6Address& dst,
        uint16_t srcPort,
        uint16_t dstPort,
        BufferInformation* bi,
        status_t* statusByte)
{
    ENTER_METHOD_SILENT();

    if (statusByte != nullptr) *statusByte = AWAIT_SENDING;
    if (dstPort > 0) {
        ContentRequest* cRequest = getContentRequest(bi, statusByte);
        if (cRequest != nullptr) {

            UDPPacket* udp = static_cast<UDPPacket*>(cRequest->content);
            ASSERT(udp!=nullptr);

            udp->setDestPort(dstPort);
            udp->setSrcPort(srcPort);
            udp->setData(bi->getContent(), bi->getSize());
            cRequest->dst = dst;

            LOG_INFO("UDP->IP:" << srcPort
                    << "->[" << dst.getAddressPart(7) << "]:" << dstPort);

            toIP.send(cRequest);
            UDP_SCALAR_INC(numReq);
            return true;
        }

        UDP_SCALAR_INC(numOutOfReqs);
        LOG_ERROR("No CReq.");
    }

    // failure cases
    if (bi != nullptr) {
        bi->free();
    }
    if (statusByte != nullptr) {
        *statusByte = FAILURE;
    }
    return false;
}

void UDPLayer::finish() {
    recordScalar("sent successful", stats.numSent);
    recordScalar("not sent", stats.numNotSent);
    recordScalar("num req", stats.numReq);
    recordScalar("num out of requests", stats.numOutOfReqs);

    for (uint8_t i = 0; i < UDP_MAX_LISTENERS; i++) {
        if (listeners[i].listener != nullptr) {
            listeners[i].listener = nullptr;
        }
    }

}

ContentRequest* UDPLayer::getContentRequest(BufferInformation* data, status_t* statusByte) {
    ContentRequest* ret = nullptr;
    uint8_t i;
    for (i = 0;
            i < cfg.numContentRequests && contentRequests[i].occupied;
            i++);
    if (i < cfg.numContentRequests) {
        LOG_DEBUG("ContentReq acquired");
        ret = &(contentRequests[i].contentRequest);
        contentRequests[i].occupied = true;
        contentRequests[i].buffer = data;
        statusBytes[i] = statusByte;
    }
    return ret;
}

void UDPLayer::freeContentRequest(ContentRequest* cr, status_t status) {
    uint8_t i;
    for (i = 0;
            i < cfg.numContentRequests &&
            &(contentRequests[i].contentRequest) != cr;
            i++);
    if (i < cfg.numContentRequests) {
        LOG_DEBUG("ContentReq released");
        TAKE_MESSAGE(&(contentRequests[i].contentRequest));
        contentRequests[i].freeHolder();
        if (statusBytes[i] != nullptr) {
            *(statusBytes[i]) = status;
            statusBytes[i] = nullptr;
        }
    }
}



void UdpConfig::doSerialize(cometos::ByteVector& buf) const {
    serialize(buf, this->numContentRequests);
}

void UdpConfig::doUnserialize(cometos::ByteVector& buf) {
    unserialize(buf, this->numContentRequests);
}

}

namespace cometos {
void serialize(ByteVector & buf, const cometos_v6::UdpLayerStats & value) {
    serialize(buf, value.numSent);
    serialize(buf, value.numNotSent);
    serialize(buf, value.numReq);
    serialize(buf, value.numOutOfReqs);
}
void unserialize(ByteVector & buf, cometos_v6::UdpLayerStats & value) {
    unserialize(buf, value.numOutOfReqs);
    unserialize(buf, value.numReq);
    unserialize(buf, value.numNotSent);
    unserialize(buf, value.numSent);
}

}
