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
 * @author: Martin Ringwelski
 */


#ifndef __COMETOS_V6_IPFORWARD_H_
#define __COMETOS_V6_IPFORWARD_H_

#include "cometos.h"

#include "RemotelyConfigurableModule.h"
#include "IPv6Request.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "NeighborDiscovery.h"
#include "RoutingBase.h"
#include "RoutingTable.h"
#ifdef COMETOS_V6_RPL_SR
#include "RPLSourceRoutingTable.h"
#endif
#include "IPv6InterfaceTable.h"
#include "PersistableConfig.h"
#include "ICMPv6.h"

#include "Pool.h"
#include "HashMap.h"

#define IPFWD_MODULE_NAME "ip"

namespace cometos_v6 {

#ifndef LOWPAN_ENABLE_DIRECT_FORWARDING
const uint8_t IPFWD_MAX_REQUESTS = 8;
#else
#include "lowpanconfig.h"
const uint8_t IPFWD_MAX_REQUESTS = LOWPAN_SET_DF_PACKETS;
#endif
const uint8_t IP_MAXCONTENTS = 8;

#if defined SWIG || defined BOARD_python
enum routeResult_t
#else
enum routeResult_t : uint8_t
#endif
{
    RR_SUCCESS,
    RR_NO_ROUTE,
    RR_NO_NEIGHBOR,
    RR_NO_MESSAGES,
    RR_SRHEADER_ERROR
};

struct IpData {
    IPv6Datagram    datagram;
    IPv6Request     ipRequest;
    ContentRequest* cr;

    IpData() :
       cr(NULL)
    {
       ipRequest.data.datagram = &datagram;
    }

    bool mapsTo(IPv6Request & req) {
        return &req == &ipRequest;
    }

};

struct IpIndicationData {
    IPv6Request*    ipRequest;
    ContentRequest  cRequest;
    IpIndicationData(): ipRequest(NULL) {}

    bool mapsTo(ContentRequest & req) {
        return &cRequest == &req;
    }
};

struct IpConfig : public cometos::PersistableConfig {
    IpConfig(uint8_t numRequestsToLower = IPFWD_MAX_REQUESTS,
             uint8_t numIndicationsToUpper = IP_MAXCONTENTS) :
                 numRequestsToLower(numRequestsToLower),
                 numIndicationsToUpper(numIndicationsToUpper)
    {}

    virtual void doSerialize(cometos::ByteVector& buf) const;

    virtual void doUnserialize(cometos::ByteVector& buf);

    uint8_t numRequestsToLower;
    uint8_t numIndicationsToUpper;

    bool operator==(const IpConfig& rhs);

    bool isValid();
};



class IpForward : public cometos::RemotelyConfigurableModule<IpConfig>
{
public:
    static const uint8_t HOP_LIMIT_DEFAULT = 16;
    static const uint8_t HOP_LIMIT_UNSPEC = 64;
    static const uint8_t HOP_LIMIT_LINKLOCAL = 1;

    IpForward(const char * service_name = IPFWD_MODULE_NAME,
            RoutingTable* rt = NULL,
#ifdef COMETOS_V6_RPL_SR
            RPLSourceRoutingTable* srt = NULL,
#endif
            RoutingBase* rb = NULL, NeighborDiscovery* nd = NULL,
            IPv6InterfaceTable* it = NULL, ICMPv6* icmp = NULL);
    ~IpForward();

    void initialize();
    void finish();

    void handleRequestFromLowpan(IPv6Request *ipRequest);
    void handleRequestFromUpper(ContentRequest *cRequest);

    void handleResponseFromLowpan(IPv6Response* ipResponse);
    void handleResponseFromUpper(ContentResponse* cResponse);

    void initRequestsToLower(IpData * req);
    void initRequestsToUpper(IpIndicationData * req);

#ifdef OMNETPP
    void finishRequestsToLower(IpData * data);
    void finishRequestsToUpper(IpIndicationData * ind);
#endif

    bool isForMe(const IPv6Address & destination);

    /**
     * Can be used by an adaption layer, i.e. 6LoWPAN to retrieve routing
     * information before a whole datagram was received. Sets the IPv6Request
     * parameter if next hop retrieval and neighbor discovery were successful.
     *
     * @param[out] ipReq if successful, will hold a pointer to an IPv6Request
     *                  when the method returns. Control over this pointer
     *                  MUST be passed back to the IpForward layer by means
     *                  of calling its response method!
     *                  Additionally, the returned ipRequest will hold
     *                  a valid pointer to an ipDatagram structure
     * @param[in]  destination destination address
     * @return  RR_SUCCESS, in case of success (route/neighbor found)
     *          RR_NO_ROUTE, if no route is found for destination
     *          RR_NO_NEIGHBOR, if neighbor for next hop could not be resolved
     *          RR_NO_MESSAGES, if message pool was empty
     */
    routeResult_t crossLayerRouting(IPv6Request* & ipReq, const IPv6Address & destination);

    virtual bool isBusy();
    virtual void applyConfig(IpConfig& cfg);
    virtual IpConfig& getActive();

//    cometos_error_t setConfig(IpConfig & cfg);
//
//    IpConfig getConfig();
//
//    IpConfig getActiveConfig();
//
//    cometos_error_t resetConfig();

    cometos::InputGate<IPv6Request>     fromLowpan;
    cometos::OutputGate<IPv6Request>    toLowpan;

    cometos::InputGate<ContentRequest>     fromUnknown;
    cometos::OutputGate<ContentRequest>    toUnknown;

    cometos::InputGate<ContentRequest>     fromUDP;
    cometos::OutputGate<ContentRequest>    toUDP;

    cometos::InputGate<ContentRequest>     fromICMP;
    cometos::OutputGate<ContentRequest>    toICMP;

    // REMOVE PRE PROCESSOR DIR BEL
#ifdef ENABLE_TESTING
    void scheduleAndPassOwnership(void*a){}
    void take(void* q){}
#endif

private:
    routeResult_t routeOver(IPv6Request *ipRequest);

    void deleteMessagePools();

    void createMessagePools(const IpConfig& cfg);

    /**
     * Tries to find a route and resolve the neighbor for the given destination
     * address.
     *
     * @param[in] destination IPv6Address of destination node
     * @param[out] link layer address of output interface (source)
     * @param[out] link layer address of destination
     * @return RR_SUCCESS if route was found and neighbor resolved (src and dst
     *                    contain link layer addresses in this case)
     *         RR_NO_ROUTE if no route was found; src and dst are invalid then
     *         RR_NO_NEIGHBOR if link layer address of next hop could not be
     *                        resolved, src and dst invalid
     */
    routeResult_t findRoute(
            const IPv6Address & destination,
            Ieee802154MacAddress & src,
            Ieee802154MacAddress & dst);

#ifdef COMETOS_V6_RPL_SR
    bool addSourceRoutingHeader(IPv6Datagram *datagram);

    //Tries to find a source Route for a given destination
    routeResult_t findSourceRoute(
            const IPv6Datagram * datagram,
            Ieee802154MacAddress & src,
            Ieee802154MacAddress & dst);
#endif

    void forwardToUpper(ContentRequest *cRequest);


    RoutingTable*          rt;
#ifdef COMETOS_V6_RPL_SR
    RPLSourceRoutingTable*    srt;
#endif
    RoutingBase*           rb;
    NeighborDiscovery*     nd;
    IPv6InterfaceTable*    it;
    ICMPv6*                icmp;

    IpConfig               cfg;

    cometos::MappedPoolBase<IpData>* poolToLower;
    cometos::MappedPoolBase<IpIndicationData>* poolToUpper;
};

}
#endif
