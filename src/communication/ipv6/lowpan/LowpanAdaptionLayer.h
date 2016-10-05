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
 * @author Martin Ringwelski, Andreas Weigel
 */

#ifndef __COMETOS_V6_LOWPANADAPTIONLAYER_H_
#define __COMETOS_V6_LOWPANADAPTIONLAYER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "lowpanconfig.h"
#include "IPv6Request.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "Pool.h"
#include "HashMap.h"
#include "RemoteModule.h"
#include "LowpanIndication.h"
#include "LowpanAdaptionLayerStructures.h"
#include "LowpanVariant.h"
#include "Rfc4944Specification.h"
#include "LFFRSpecification.h"
#include "ParameterStore.h"
#include "RemotelyConfigurableModule.h"
#include "LocalCongestionAvoider.h"


/*TYPES----------------------------------------------------------------------*/



namespace cometos_v6 {

class LowpanAdaptionLayerSim;

/**
 * \class  LowpanAdaptionLayer
 * \brief  Module for Adapting the 6LoWPAN Mechanisms to use IPv6 over 802.15.4.
 *
 * This Module compresses the IPv6 Header and Fragments the Packet to send it
 * over an IEEE 802.15.4 connection. It also reassembles and decompresses
 * Packets received over that connection.
 */
class LowpanAdaptionLayer : public cometos::RemotelyConfigurableModule<LowpanConfig>
{
    friend LowpanAdaptionLayerSim;

public:
    BufferInformation* getLowpanDataBuffer(uint16_t size);

    BufferInformation* getContainingLowpanDataBuffer(const uint8_t* ptr);

    timeOffset_t getSendDelay() const;
    static const uint16_t DURATION_EWMA_ONE = 200;
    static const uint16_t DURATION_EWMA_ALPHA = 150;
    static const uint16_t DURATION_MAX = 200;

    /**
     * The constructor receives a name and pointers to needed modules.
     * All Parameters are optional. If no pointers to the needed modules are
     * given, this module tries to find them during initialization by their
     * default names.
     *
     * @param   *service_name   Name of the Module
     * @param   *nd             Pointer to the Neighbordiscovery module
     * @param   *rt             Pointer to the RoutingTable module
     * @param   *icmp           Pointer to the ICMP module
     */
    LowpanAdaptionLayer(const char * service_name = NULL);

    ~LowpanAdaptionLayer();

    virtual void initialize();

    /**
     * Initializion routine for messages in message pool
     */
    void initRequestsToUpper(IPv6Request_Content_t * req);

    void finishRequestsToUpper(IPv6Request_Content_t * req);

    void finish();

    LowpanAdaptionLayerStats getStats();

    // inherited pure virtual methods from RemotelyConfigurableModule
    virtual bool isBusy();
    virtual void applyConfig(LowpanConfig& cfg);
    virtual LowpanConfig& getActive();


    void resetStats();

    /**
     * Checks for timeouts of fragments in the buffer.
     *
     * @param *msg  Pointer to the schedule message. Not used.
     */
    void fragmentTimeout(cometos::Message *msg);

    /**
     * Handles an IPv6Request from the IP Layer.
     * Receives an IP request and compresses the header. After compression the
     * createFrame() method is called to do further fragmentation, if needed.
     *
     * @param *iprequest    the IP request from the IP Layer
     */
    void handleIPRequest(IPv6Request *iprequest);

    /**
     * Handles a DataIndication from the MAC Layer.
     * Receives a MAC frame and forwards it to the analyzePacket method.
     *
     * @param *ind   the MAC request from the MAC Layer
     */
    void handleMACIndication(LowpanIndication *ind);

    /**
     * Handles a snooped Lowpan frame.
     * @param ind    pointer to snooped lowpan frame
     */
    void handleSnoopedLowpanFrame(LowpanIndication* ind);

    /**
     * Handles responses from the IP Layer.
     * After a response, the buffer can be freed and the message reused.
     *
     * @param *resp     Response from the IP Layer
     */
    void receiveIPResponse(IPv6Response *resp);

    /**
     * Handles responses from the MAC Layer.
     * After a response, the next frame can be sent.
     *
     * @param *resp     Response from the MAC Layer
     */
    void receiveMACResponse(cometos::DataResponse *resp);

    bool enqueueQueueObject(QueueObject * obj);

    void eraseQueueObject();

    void rateTimerFired(cometos::Message * msg);

    void RTOTimeout(cometos::Message *msg);

    void sendIfAllowed();

    bool isSuccessorOfActiveDg(const QueueObject& obj);

    cometos::InputGate<IPv6Request> fromIP; ///< Gate from the IP Layer
    cometos::InputGate<LowpanIndication> fromMAC; ///< Gate from the MAC Layer
    cometos::InputGate<LowpanIndication> macSnoop; //< Snooping gate from the MAC Layer

    cometos::OutputGate<IPv6Request> toIP; ///< Gate to the IP Layer
    cometos::OutputGate<cometos::DataRequest> toMAC; ///< Gate to the MAC Layer

    //RemoveThis Geogin:
#ifdef ENABLE_TESTING
    void scheduleAndPassOwnership(void* a){};
    void take(void* a){};
#endif
private:

    template<typename T>
    bool isnull(T* ptr) {
        return (ptr == NULL) || (ptr == nullptr);
    }

    void deleteHandlerObjects();

    void createHandlerObjects(const LowpanConfig& cfg);

    virtual void createQueueAndLca(const LowpanConfig& cfg);

    virtual void createAndInitializeMessagePools(const LowpanConfig& cfg);

    virtual void createSpecificationImplementation(const LowpanConfig& cfg);

    static const uint16_t DEFAULT_AVG_PKT_DURATION = 6 << LowpanAdaptionLayerStats::DURATION_FACTOR_SHIFT;

    uint16_t fixedAvgPktDuration;

    /** Parses and removes a 6LoWPAN mesh header from a given frame.
     * @param[in,out] frame
     *      airframe containing the 6LoWPAN mesh header;
     * @param[in] dispatch
     *      mesh header dispatch byte
     * @param[out] src
     *      will be set to the contained source address
     * @param[out] dst
     *      will be set to the contained destination address
     * @return dispatch byte of the next 6LoWPAN header in the frame
     */
    uint8_t checkMesh(cometos::Airframe& frame,
                      uint8_t dispatch,
                      Ieee802154MacAddress& src,
                      Ieee802154MacAddress& dst);

    /*void checkFragmentation(cometos::Airframe& frame, uint8_t head,
            Ieee802154MacAddress& src, Ieee802154MacAddress& dst);
    void checkIP(cometos::Airframe& frame, uint8_t head,
            Ieee802154MacAddress& src, Ieee802154MacAddress& dst);

    void updateStatsOnReturnValue(bufStatus_t retVal);*/

    void sendNextFrame();

    void saveIPContextandSendtoIpLayer(IPv6DatagramInformation & contextInfo,
            Ieee802154MacAddress& src, Ieee802154MacAddress& dst);

    cometos::Message timeOutSchedule;
    cometos::Message rateTimer;
    cometos::Message _RTOTimer;


    uint16_t    fragmentTagOwn;

    /** Pointer to the queue */
    LowpanQueue* queue;

    /** Byte buffer for sending/reassembling/forwarding */
    ManagedBuffer*  buffer;

    /** Handler for incoming fragments of 6lowpan-fragmented datagrams */
    FragmentHandler* fragmentHandler;

    /** Buffer to reassemble datagrams destined to us or in Assembly Mode*/
    AssemblyBufferBase* fragmentBuffer;

    IPv6Context srcContexts[16];
    IPv6Context dstContexts[16];

    LocalCongestionAvoider* lca;

    retransmissionList _ipRetransmissionList;

    bool    meshUnder;

    bool sending;

    bool queueStarving;

    time_ms_t firstEmpty;

    bool switchedQo;
    uint8_t numTimesQueueEmpty;

    /** Message pool for IP data indications to upper layer */
    cometos::MappedPoolBase<IPv6Request_Content_t>* poolToUpper;

    LAL_VECTOR(QueueSizeVector);
    LAL_VECTOR(BufferSizeVector);
    LAL_VECTOR(BufferNumElemVector);
    LAL_VECTOR(FreeIPRequestsVector);
    LAL_VECTOR(MacFrameDurationVector);

    LowpanConfig cfg;
    LowpanAdaptionLayerStats stats;
    time_ms_t tsSend;
    LowpanVariant* lowpanVariant;

    NeighborDiscovery* nd;
};

} // namespace cometos_v6

namespace cometos {
    uint8_t mac_getMaximumPayload();

    void serialize(ByteVector & buf, const cometos_v6::LowpanAdaptionLayerStats & val);
    void unserialize(ByteVector & buf, cometos_v6::LowpanAdaptionLayerStats & val);
}


#endif
