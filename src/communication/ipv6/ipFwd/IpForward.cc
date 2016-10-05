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

#include "IpForward.h"
#include "RoutingTable.h"
#include "ICMPv6Message.h"
#include "UDPPacket.h"
#include "palId.h"
#include "palLed.h"
#include "IPv6RoutingHeader.h"
#include "ParameterStore.h"

#ifdef COMETOS_V6_RPL
#include "RPLRouting.h"
typedef cometos_v6::RPLRouting COMETOS_v6_ROUTINGTYPE;
#else
#include "StaticRouting.h"
typedef cometos_v6::StaticRouting COMETOS_v6_ROUTINGTYPE;
#endif

namespace cometos_v6 {

const uint8_t IpForward::HOP_LIMIT_DEFAULT;
const uint8_t IpForward::HOP_LIMIT_UNSPEC;
const uint8_t IpForward::HOP_LIMIT_LINKLOCAL;

Define_Module(IpForward);

bool IpConfig::operator==(const IpConfig& rhs) {
    return numIndicationsToUpper == rhs.numIndicationsToUpper
            && numRequestsToLower == rhs.numRequestsToLower;
}

void IpConfig::doSerialize(cometos::ByteVector& buf) const{
    serialize(buf, numIndicationsToUpper);
    serialize(buf, numRequestsToLower);
}

void IpConfig::doUnserialize(cometos::ByteVector& buf) {
    unserialize(buf, numRequestsToLower);
    unserialize(buf, numIndicationsToUpper);
}

bool IpConfig::isValid() {
    return numIndicationsToUpper <= 40
            && numRequestsToLower <= 40;
}

IpForward::IpForward(const char * service_name, RoutingTable* rt,
#ifdef COMETOS_V6_RPL_SR
        RPLSourceRoutingTable* srt,
#endif
        RoutingBase* rb, NeighborDiscovery* nd,
        IPv6InterfaceTable* it, ICMPv6* icmp) :
        cometos::RemotelyConfigurableModule<IpConfig>(service_name),
        fromLowpan(this, &IpForward::handleRequestFromLowpan, "fromLowpan"),
        toLowpan(this, "toLowpan"),
        fromUnknown(this, &IpForward::handleRequestFromUpper, "fromUnknown"),
        toUnknown(this, "toUnknown"),
        fromUDP(this, &IpForward::handleRequestFromUpper, "fromUDP"),
        toUDP(this, "toUDP"),
        fromICMP(this, &IpForward::handleRequestFromUpper, "fromICMP"),
        toICMP(this, "toICMP"),
        rt(rt),
#ifdef COMETOS_V6_RPL_SR
        srt(srt),
#endif
        rb(rb),
        nd(nd),
        it(it),
        icmp(icmp),
        poolToLower(nullptr),
        poolToUpper(nullptr)
{}

IpForward::~IpForward() {
    deleteMessagePools();
}

void IpForward::initialize() {
    RemotelyConfigurableModule<IpConfig>::initialize();
    if (nd == NULL) {
        nd = static_cast<NeighborDiscovery*>(getModule(NEIGHBOR_DISCOVERY_MODULE_NAME));
    }
    if (rt == NULL) {
        rt = static_cast<RoutingTable*>(getModule(ROUTING_TABLE_MODULE_NAME));
    }
#ifdef COMETOS_V6_RPL_SR
    if (srt == NULL) {
        srt = static_cast<RPLSourceRoutingTable*>(getModule(SOURCE_ROUTING_TABLE_MODULE_NAME));
    }
#endif
    if (it == NULL) {
        it = static_cast<IPv6InterfaceTable*>(getModule(INTERFACE_TABLE_MODULE_NAME));
    }
    if (icmp == NULL) {
        icmp = static_cast<ICMPv6*>(getModule(ICMP_MODULE_NAME));
    }
    if (rb == NULL) {
        rb = static_cast<COMETOS_v6_ROUTINGTYPE *>(getModule(ROUTING_MODULE_NAME));
    }

    ASSERT(rb != NULL);
    ASSERT(nd != NULL && rt != NULL && it != NULL && icmp != NULL && rb != NULL);

    /** retrieve persisted config */
    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
    if (ps != nullptr) {
        ps->getCfgData(this, cfg);
    }

//    remoteDeclare(&IpForward::setConfig, "sc");
//    remoteDeclare(&IpForward::getConfig, "gc");
//    remoteDeclare(&IpForward::resetConfig, "rc");
//    remoteDeclare(&IpForward::getActiveConfig, "gac");

    CONFIG_NED_OBJ(cfg, numRequestsToLower);
    CONFIG_NED_OBJ(cfg, numIndicationsToUpper);

    createMessagePools(cfg);

}

void IpForward::finish() {
#ifdef OMNETPP
    poolToLower->finish();
    poolToUpper->finish();
#endif
}

void IpForward::initRequestsToLower(IpData * req) {
    req->ipRequest.setResponseDelegate(createCallback(&IpForward::handleResponseFromLowpan));
    req->cr = NULL;
    ASSERT(req->ipRequest.data.datagram == &(req->datagram));
}

void IpForward::initRequestsToUpper(IpIndicationData * ind) {
    ind->cRequest.setResponseDelegate(createCallback(&IpForward::handleResponseFromUpper));
    ind->cRequest.content = NULL;
    ind->cRequest.src.set(0,0,0,0,0,0,0,0);
    ind->cRequest.dst.set(0,0,0,0,0,0,0,0);
}


#ifdef OMNETPP
void IpForward::finishRequestsToLower(IpData * data) {
    take(&(data->ipRequest));
    const FollowingHeader* th = data->datagram.getLastHeader();
    if (th != NULL) {
        if (th->getHeaderType() == UDPPacket::HeaderNumber ||
                th->getHeaderType() == ICMPv6Message::HeaderNumber) {
            data->datagram.removeHeader(-1, false);
        }
    }
}

void IpForward::finishRequestsToUpper(IpIndicationData * ind) {
    take(&ind->cRequest);
}
#endif


void IpForward::deleteMessagePools() {
    delete poolToUpper;
    poolToUpper = nullptr;
    delete poolToLower;
    poolToLower = nullptr;
}

void IpForward::createMessagePools(const IpConfig & cfg) {
    ASSERT(poolToLower == nullptr);
    ASSERT(poolToUpper == nullptr);
    poolToLower = new cometos::DynMappedPool<IpData>(createCallback(&IpForward::initRequestsToLower),
#ifdef OMNETPP
               createCallback(&IpForward::finishRequestsToLower),
#endif
               cfg.numRequestsToLower);

    poolToUpper = new cometos::DynMappedPool<IpIndicationData>(createCallback(&IpForward::initRequestsToUpper),
#ifdef OMNETPP
               createCallback(&IpForward::finishRequestsToUpper),
#endif
               cfg.numIndicationsToUpper);
    poolToLower->initialize();
    poolToUpper->initialize();
}


bool IpForward::isBusy() {
    if (poolToUpper->size() != poolToUpper->maxSize() ||
            poolToLower->size() != poolToLower->maxSize()) {
        LOG_ERROR("missing msg; ptus=" << (int) poolToUpper->size()
                  << "|ptums=" << (int) poolToUpper->maxSize()
                  << "|ptls=" << (int) poolToLower->size()
                  << "|ptlms=" << (int) poolToLower->maxSize());
        return true;
    } else {
        return false;
    }
}

void IpForward::applyConfig(IpConfig& cfg) {
    this->cfg = cfg;

    deleteMessagePools();
    createMessagePools(this->cfg);
}

IpConfig& IpForward::getActive() {
    return this->cfg;
}

//cometos_error_t IpForward::setConfig(IpConfig & newCfg) {
//
//    // first make sure that no messages are flying around somewhere
//    if (poolToUpper->size() != poolToUpper->maxSize() ||
//            poolToLower->size() != poolToLower->maxSize()) {
//        LOG_ERROR("missing msg; ptus=" << (int) poolToUpper->size()
//                  << "|ptums=" << (int) poolToUpper->maxSize()
//                  << "|ptls=" << (int) poolToLower->size()
//                  << "|ptlms=" << (int) poolToLower->maxSize());
//        return COMETOS_ERROR_BUSY;
//    }
//
//    if (newCfg.isValid()) {
//
//        // if pools are currently not used, we can delete and recreate them, and
//        // persist the configuration data for a reset
//        cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
//        newCfg.setPersistent(false);
//        if (ps != nullptr) {
//            ps->setCfgData(this, newCfg);
//        }
//
//        cfg = newCfg;
//
//        deleteMessagePools();
//        createMessagePools(cfg);
//
//        return COMETOS_SUCCESS;
//    } else {
//        return COMETOS_ERROR_INVALID;
//    }
//}
//
//
//IpConfig IpForward::getConfig() {
//    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
//    IpConfig storedCfg;
//    if (ps != NULL) {
//        ps->getCfgData(this, storedCfg);
//    } else {
//        storedCfg.setPersistent(false);
//    }
//    storedCfg.setEqualToActiveConfig(storedCfg == cfg);
//
//    return storedCfg;
//}
//
//IpConfig IpForward::getActiveConfig() {
//    return cfg;
//}
//
//
//cometos_error_t IpForward::resetConfig() {
//    cometos_error_t result = COMETOS_SUCCESS;
//    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
//    if (ps != NULL) {
//        result = ps->resetCfgData(this);
//    }
//    return result;
//}




void IpForward::handleRequestFromLowpan(IPv6Request *reqFromLower) {
    LOG_DEBUG("Rcvd IPRq frm Lowpan" <<"ip dst: " << reqFromLower->data.datagram->dst.str() << " ip src: " <<reqFromLower->data.datagram->src.str());
    LOG_DEBUG("Datagram Datalength: " << reqFromLower->data.datagram->getUpperLayerPayloadLength());

    if (reqFromLower->has<LlRxInfo>()) {
        rb->rxResult(reqFromLower->data.datagram->src, *reqFromLower->get<LlRxInfo>());
    }

    if (isForMe(reqFromLower->data.datagram->dst)) {

        // TODO: Multicast support

        IpIndicationData * ipInd = poolToUpper->get();
        if (ipInd != NULL) {

            // remember original request
            ipInd->ipRequest = reqFromLower;
            ContentRequest & cRequest = ipInd->cRequest;
            cRequest.src = reqFromLower->data.datagram->src;
            cRequest.dst = reqFromLower->data.datagram->dst;
            cRequest.content = reqFromLower->data.datagram->getLastHeader();

//            if (reqFromLower->has<LlRxInfo>()) {
//                cRequest.set(reqFromLower->get<LlRxInfo>());
//            }

            forwardToUpper(&cRequest);
        } else {
            LOG_ERROR("No CReq");
            reqFromLower->response(new IPv6Response(reqFromLower, IPv6Response::IPV6_RC_OUT_OF_UPPER_MSGS));
        }
    } else {
        reqFromLower->data.datagram->decrHopLimit();
        if (reqFromLower->data.datagram->getHopLimit() > 0) {
            IpData * newData = poolToLower->get();
            if (newData != NULL) {
                ASSERT(newData->cr == NULL);
                newData->cr = NULL;
                IPv6Request * newReq = &(newData->ipRequest);

                // TODO FIXME we workaround a missing assignment operator here
                // to prevent memory access violations
                newReq->data.datagram->dst = reqFromLower->data.datagram->dst;
                newReq->data.datagram->src = reqFromLower->data.datagram->src;
                newReq->data.datagram->setFlowLabel(reqFromLower->data.datagram->getFlowLabel());
                newReq->data.datagram->setTrafficClass(reqFromLower->data.datagram->getTrafficClass());
                newReq->data.datagram->setHopLimit(reqFromLower->data.datagram->getHopLimit());
                LOG_DEBUG((uintptr_t) reqFromLower->data.datagram->getNextHeader() << " " << (uintptr_t) newReq->data.datagram->getNextHeader());
                newReq->data.datagram->setNextHeader(reqFromLower->data.datagram->decapsulateAllHeaders());
                LOG_DEBUG((uintptr_t) reqFromLower->data.datagram->getNextHeader() << " " << (uintptr_t) newReq->data.datagram->getNextHeader());
                routeResult_t rr = routeOver(newReq);
                if (rr == RR_SUCCESS) {
                    reqFromLower->response(new IPv6Response(reqFromLower, IPv6Response::IPV6_RC_SUCCESS, false));
                } else {
                    reqFromLower->response(new IPv6Response(reqFromLower, IPv6Response::IPV6_RC_NO_ROUTE));
                }
            } else {
                reqFromLower->response(new IPv6Response(reqFromLower, IPv6Response::IPV6_RC_OUT_OF_LOWER_MSGS));
            }

        } else {
            LOG_WARN("Pkt rcd HpLt");
            LOG_DEBUG("Send ICMP ErrMsg");
            icmp->sendErrorMessage(
                    reqFromLower->data.datagram,
                    ICMP_TYPE_TIME_EXCEEDED,
                    0); // TODO If it was a fragment of a bigger IP Packet Code 1
            reqFromLower->response(new IPv6Response(reqFromLower, IPv6Response::IPV6_RC_OUT_OF_HOPS));
        }
    }
}

void IpForward::handleRequestFromUpper(ContentRequest *cRequest) {
    ASSERT(cRequest != NULL);
    LOG_DEBUG("up->IP CReq");
    if (isForMe(cRequest->dst)) {
        LOG_DEBUG("Pckt for me! " << cRequest->dst.str());
        if (cRequest->dst.isMulticast()) {
            cRequest->src = it->getInterface(0).getLocalAddress();

            // TODO we send a dynamically created copy of the original content request
            //      back to the node if the given address is a loopback or registered
            //      multicast address of this node; there might be a more beautiful solution
                IpIndicationData * ipInd = poolToUpper->get();
                if (ipInd != NULL) {
                    ipInd->ipRequest = NULL;
                    ContentRequest & newCRequest = ipInd->cRequest;
                    newCRequest.src = cRequest->src;
                    newCRequest.dst = cRequest->dst;
                    newCRequest.content = cRequest->content->cloneAll();
                    newCRequest.content->generateChecksum(cRequest->src, cRequest->dst);
                    LOG_INFO("Loopback");
                    forwardToUpper(&newCRequest);
                } else {
                    LOG_ERROR("No CReq to upper");
                    //ASSERT(false);
                }

            LOG_INFO("Multicast");

            if (cRequest->dst.getAddressPart(0) == 0xff02 ||
                    cRequest->dst.getAddressPart(0) == 0xff05)
            {
                IpData* ipdata = poolToLower->get();
                if (ipdata != NULL) {
                    IPv6Request * iprequest = &(ipdata->ipRequest);
                    ipdata->cr = cRequest;
                    iprequest->data.datagram->addHeader(cRequest->content);

                    iprequest->data.datagram->src = cRequest->src;
                    iprequest->data.datagram->setHopLimit(HOP_LIMIT_LINKLOCAL);


                    iprequest->data.datagram->dst = cRequest->dst;
                    cRequest->content->generateChecksum(iprequest->data.datagram->src,
                            iprequest->data.datagram->dst);


                    LOG_DEBUG("Sending IP Pckt frm "
                            << iprequest->data.datagram->src.getAddressPart(7)
                            << " to "
                            << iprequest->data.datagram->dst.getAddressPart(7)
                            << " HopLimit "
                            << (uint16_t) iprequest->data.datagram->getHopLimit());

                       const Ieee802154MacAddress* dstMACAddr = nd->resolveNeighbor(iprequest->data.datagram->dst);
                    LOG_DEBUG("dstMACAddr: " << (int) dstMACAddr->a4());

                    iprequest->data.srcMacAddress =
                            it->getInterface(0).getMacAddress();

                    iprequest->data.dstMacAddress = *dstMACAddr;
                    LOG_DEBUG("LowPan send");
                    // REMOVE PRE PROCESSOR DIR BEL
#ifndef ENABLE_TESTING
                    LOG_DEBUG("Pass mc @" << (uintptr_t) iprequest);
                    toLowpan.send(iprequest);
#endif
                    return;
                } else {
                    LOG_ERROR("Out of Msgs");
                    cRequest->response(new ContentResponse(cRequest, false));
                }
            } else {
                LOG_INFO("Only loopback brdcst");
                cRequest->response(new ContentResponse(cRequest, true));
            }
        } else {

            // not multicast TODO do we need a reponse to the originator?
            cRequest->src = cRequest->dst;
            forwardToUpper(cRequest);
        }

    } else {  // I'm not the destination
        LOG_DEBUG("HERE");
        IpData* ipdata = poolToLower->get();
        if (ipdata != NULL) {
            IPv6Request * ipreq = &(ipdata->ipRequest);
            ipdata->cr = cRequest;

            ipreq->data.datagram->addHeader(cRequest->content);

            if (cRequest->src.isUnspecified()) {
                if (cRequest->dst.isLinkLocal()) {
                    LOG_DEBUG("Set Src Addr to Lcl "
                            << it->getInterface(0).getLocalAddress().getAddressPart(7));

                    cRequest->src = it->getInterface(0).getLocalAddress(); // TODO using fixed interface
                    ipreq->data.datagram->setHopLimit(HOP_LIMIT_LINKLOCAL);
                } else {
                    LOG_DEBUG("Set Src Addr to Gbl "
                            << it->getInterface(0).getGlobalAddress().getAddressPart(7));

                    cRequest->src = it->getInterface(0).getGlobalAddress(); // TODO using fixed interface
                    ipreq->data.datagram->setHopLimit(HOP_LIMIT_DEFAULT);
                }
            } else {
                if (!(cRequest->dst.isLinkLocal())) {       // added a check to determine whether CRequest is supposed reach a global IP address
                                                            // if so, change src to GlobalAddress instead of LocalAddress
                                                            // Required for CoAPMedicController in order to work with Medic-Server
                    LOG_DEBUG("Request to Gbl, set Src Addr to Gbl"
                            << it->getInterface(0).getGlobalAddress().getAddressPart(7));
                    cRequest->src = it->getInterface(0).getGlobalAddress(); // TODO using fixed interface
                    ipreq->data.datagram->setHopLimit(HOP_LIMIT_DEFAULT);
                } else {
                    LOG_DEBUG("Keep Addr: ");
                    ipreq->data.datagram->setHopLimit(HOP_LIMIT_UNSPEC);
                }
//                LOG_DEBUG("Keep Addr: ");
//                ipreq->datagram->setHopLimit(HOP_LIMIT_UNSPEC);
            }
            ipreq->data.datagram->src = cRequest->src;
            ipreq->data.datagram->dst = cRequest->dst;

            cRequest->content->generateChecksum(ipreq->data.datagram->src,
                    ipreq->data.datagram->dst);

            LOG_DEBUG("Sending IP Pckt frm "
                    << ipreq->data.datagram->src.getAddressPart(7)
                    << " to "
                    << ipreq->data.datagram->dst.getAddressPart(7)
                    << " HopLimit "
                    << (uint16_t) ipreq->data.datagram->getHopLimit());

            routeOver(ipreq);
        } else {
            cRequest->response(new ContentResponse(cRequest, false));
        }
    }
}

void IpForward::handleResponseFromLowpan(IPv6Response* ipResponse) {
    if (ipResponse->success == IPv6Response::IPV6_RC_SUCCESS) {
        LOG_INFO("IP->low succ");
    } else {
        LOG_WARN("IP->low unsucc: " << (int)ipResponse->success);
    }

    IPv6Request * originalReq = ipResponse->refersTo;
    LOG_DEBUG("To req@" << (uintptr_t) (ipResponse->refersTo));
    ASSERT(originalReq != NULL);
    ASSERT(originalReq->data.datagram != NULL);

    if (ipResponse->has<LlTxInfo>()) {
        LlTxInfo info = *(ipResponse->get<LlTxInfo>());
        //LOG_INFO(rb << " " << (uintptr_t) originalReq);
        rb->txResult(originalReq->data.datagram->dst, info);
        //LOG_INFO("2:" << rb << " " << (uintptr_t) originalReq);
    }

    // find ipdata for the response's original request
    IpData * originalData = poolToLower->find(*originalReq);

    ASSERT(originalData != NULL);

    ContentRequest* cr = originalData->cr;

#ifdef OMNETPP
    take(originalReq);
#endif

    // TODO here we currently have to distinguish between responses
    // which refer to request that were caused by upper layer requests and
    // requests that were caused by ip routing. In the former case, we
    // have to remove (but not delete) the final header and pass it back
    // to the upper layer which might use it. In the latter case, all
    // headers were created dynamically and have to be removed and deleted here

    // if there is a content request associated with this response, remove
    // the last header, which must be the original header provided by the
    // content request from the upper layer and pass it back to them
    if (cr != NULL) {
        FollowingHeader * tmp = originalReq->data.datagram->decapsulateHeader(-1);
        if(tmp != cr->content){
            return;
        }
        LOG_DEBUG("Inform Higher Layer. cr:" << (uintptr_t)cr);
        cr->response(new ContentResponse(cr, ipResponse->success == IPv6Response::IPV6_RC_SUCCESS ? true : false));
    } else {
        LOG_INFO("No CReq for Resp");
    }

    //Free the memory of the addressdata pointer
    //But only if the data was added by me
    // We take the data from the source Routing Header
    FollowingHeader* fh = originalData->datagram.getNextHeader();
    while (fh != NULL && fh->getHeaderType() != IPv6RoutingHeader::HeaderNumber) {
        fh = fh->getNextHeader();
    }
    if(fh != NULL) {
        IPv6RoutingHeader * SRHeader = (IPv6RoutingHeader *)fh;
        if (SRHeader->dataAdded==1) {
            const uint8_t* addressData = SRHeader->getHData();
            delete addressData;
            SRHeader->dataAdded=0;
        }
    }

    // remove all extension headers
    while (originalReq->data.datagram->getNextHeader() != NULL) {
        originalReq->data.datagram->removeHeader(0);
    }

    // give data back to pool and clean up response
    poolToLower->putBack(originalData);
    delete ipResponse;
}

void IpForward::handleResponseFromUpper(ContentResponse* cResponse) {
    if (cResponse->success) {
        LOG_INFO("IP->up succ");
    } else {
        LOG_WARN("IP->up unsucc");
    }

    ContentRequest * originalReq = cResponse->refersTo;
    ASSERT(originalReq != NULL);

#ifdef OMNETPP
    take(originalReq);
#endif

    IpIndicationData* ipInd = poolToUpper->find(*originalReq);
    ASSERT(ipInd != NULL);
    if (ipInd->ipRequest != NULL) {
        IPv6Request* ipReq = ipInd->ipRequest;

        LOG_DEBUG("Return CntntRq. IPv6Rq " << (uintptr_t) ipReq);

        LOG_DEBUG("Snd Rsp");
        IPv6Response::ipv6ResponseCode_t rc;
        if (cResponse->success) {
            rc = IPv6Response::IPV6_RC_SUCCESS;
        } else {
            rc = IPv6Response::IPV6_RC_FAIL_FROM_UPPER;
        }
        ipReq->response(new IPv6Response(ipReq, rc));
    } else {
        LOG_INFO("No IPReq found");
    }

    // clean up previously occupied messages
    poolToUpper->putBack(ipInd);
    delete cResponse;
}

bool IpForward::isForMe(const IPv6Address & destination) {
    return rt->isLocalAddress(destination);
}

routeResult_t IpForward::crossLayerRouting(
        IPv6Request* & ipReq,
        const IPv6Address & destination) {
#ifdef OMNETPP
    Enter_Method_Silent();
#endif
    IpData * ipdata = poolToLower->get();
    if (ipdata == NULL) {
        return RR_NO_MESSAGES;
    } else {
        ipdata->cr = NULL;
        ipReq = &(ipdata->ipRequest);
        routeResult_t res = findRoute(destination, ipReq->data.srcMacAddress, ipReq->data.dstMacAddress);
        if (res == RR_SUCCESS) {
            LOG_DEBUG("Pass req@" << (uintptr_t) ipReq);
            // everything went fine, no action needed
        } else {
            ipReq = NULL;
            poolToLower->putBack(ipdata);
        }
        return res;
    }
}


routeResult_t IpForward::routeOver(IPv6Request *iprequest) {
    LOG_DEBUG("Route over");
    routeResult_t rr;

#ifdef COMETOS_V6_RPL_SR
    RPLRouting* Routing_Info = (RPLRouting*)rb;
    if (Routing_Info->nonstoring) {
        LOG_DEBUG("Using Non Storing Mode");
        //In Non-Storing mode, the root builds a strict source routing header,
        //hop-by-hop, by recursively looking up one-hop information that ties a
        //Target (address or prefix) and a transit address together.
        bool SRHAdded = false;
        if (Routing_Info->isRoot) {
            SRHAdded = addSourceRoutingHeader(iprequest->data.datagram);
            if (SRHAdded) {
                LOG_DEBUG ("Source Routing Header added successfully");
            }
            else {
                LOG_DEBUG ("Source Routing Header could not be added");
                rr = RR_SRHEADER_ERROR;
            }
        }
        if (rr!= RR_SRHEADER_ERROR  && SRHAdded) {
            rr = findSourceRoute(iprequest->data.datagram, iprequest->data.srcMacAddress, iprequest->data.dstMacAddress);
        }
        else {
            rr = findRoute(iprequest->data.datagram->dst, iprequest->data.srcMacAddress, iprequest->data.dstMacAddress);
        }
    } else {
        rr = findRoute(iprequest->data.datagram->dst, iprequest->data.srcMacAddress, iprequest->data.dstMacAddress);
    }
#else
    rr = findRoute(iprequest->data.datagram->dst, iprequest->data.srcMacAddress, iprequest->data.dstMacAddress);
#endif

    if (rr == RR_NO_ROUTE) {
        LOG_WARN("No route found");
        iprequest->response(new IPv6Response(iprequest, IPv6Response::IPV6_RC_NO_ROUTE));

        if (!isForMe(iprequest->data.datagram->src)) {
            icmp->sendErrorMessage(
                    iprequest->data.datagram,
                    ICMP_TYPE_DESTINATION_UNREACHABLE,
                    0);
        }
    } else if (rr == RR_SRHEADER_ERROR) {
        LOG_WARN("Error in Source Routing Header");
        // The "Error in Source Routing Header" message has the
        // same format as the "Destination Unreachable Message"
        iprequest->response(new IPv6Response(iprequest, IPv6Response::IPV6_RC_NO_ROUTE));
        if (!isForMe(iprequest->data.datagram->src)) {
            icmp->sendErrorMessage(
                    iprequest->data.datagram,
                    ICMP_TYPE_DESTINATION_UNREACHABLE,
                    0);
        }
    } else if (rr == RR_SUCCESS){
        LOG_DEBUG("IPv6Rq Ptr: " << (uintptr_t) iprequest);
        // neighbor found
        // REMOVE PRE PROCESSOR DIR BEL
#ifndef ENABLE_TESTING
        toLowpan.send(iprequest);
#endif
    } else if (rr == RR_NO_NEIGHBOR) {
        LOG_WARN("NO_NEIGHBOR");
        iprequest->response(new IPv6Response(iprequest, IPv6Response::IPV6_RC_NO_ROUTE));
    }
    return rr;
}

#ifdef COMETOS_V6_RPL_SR
bool IpForward::addSourceRoutingHeader(IPv6Datagram *sourceDatagram) {
    LOG_DEBUG("Try to add SrcRouting Header");

    IPv6RoutingHeader* sourceRoutingHeader = new IPv6RoutingHeader;
    //sourceDatagram->createNextHeader(43); ?
    //Source Routing Headers have Routing Type 3
    sourceRoutingHeader->setRoutingType(3);

    IPv6Address dest = sourceDatagram->dst;
    const IPv6Route * sourceRoute = srt->doLongestPrefixMatch(dest);
    if (!sourceRoute){
        LOG_DEBUG("No source route to the destination");
        return 0;
    }

    IPv6Address nextHop = sourceRoute->getNextHop();

    // For now we store the hops addresses here
    IPv6Address hops[IP_MAXCONTENTS];
    int i = 0;

    while (nextHop != dest) {
        if (i==IP_MAXCONTENTS) {
            //Too many hops, cannot add the Header
            return 0;
        }
        hops[i] = nextHop;
        nextHop = sourceRoute->getNextHop();

        i++;
    }
    int numberHops = i;

    //Now we store the hops in the Header
    uint8_t* addressData = new uint8_t[numberHops * IP6_BYTES];
    int j=0;
    for (int index=0;index<numberHops;index++) {
        hops[index].writeAddress(&(addressData[j]));
        j += IP6_BYTES;
    }
    sourceRoutingHeader->setData(addressData,j);
    sourceRoutingHeader->dataAdded=1;
    sourceRoutingHeader->setSegmentsLeft(numberHops-1);

    //We add the Header to the datagram
    //We look for the right position to add the Header
    FollowingHeader* fh = sourceDatagram->getNextHeader();
    uint8_t pos = 0;
    while (fh != NULL) {
        switch (fh->getHeaderType()) {
        case IPv6HopByHopOptionsHeader::HeaderNumber:
            pos++;
            fh = fh->getNextHeader();
            break;
        case IPv6RoutingHeader::HeaderNumber:
            pos++;
            fh = fh->getNextHeader();
            break;
        case IPv6FragmentHeader::HeaderNumber:
            pos++;
            fh = fh->getNextHeader();
            break;
        case IPv6DestinationOptionsHeader::HeaderNumber:
            pos++;
            fh = fh->getNextHeader();
            break;
        default:
            fh = NULL;
        }
    }
    sourceDatagram->addHeader(sourceRoutingHeader,pos);
    return 1;
}
#endif

routeResult_t IpForward::findRoute(
        const IPv6Address & destination,
        Ieee802154MacAddress & src,
        Ieee802154MacAddress & dst) {

    LOG_DEBUG("Srch for Rt Dst " << destination.str());
    const IPv6Route * route = rt->doLongestPrefixMatch(destination);
    LOG_DEBUG("Check Rt");
    if (!destination.isLinkLocal() && route == NULL) {
        LOG_WARN("No Rt to Dst!");
        return RR_NO_ROUTE;
    } else {
        LOG_DEBUG("Get MAC Addr");
        IPv6Address nh;
        if (destination.isLinkLocal() || route->getNextHop().isUnspecified()) {
            nh = destination;
        } else {
            nh = route->getNextHop();
        }
        const Ieee802154MacAddress* dstMACAddr = nd->resolveNeighbor(nh);
        if (dstMACAddr != NULL) {
            src = it->getInterface(route->getInterfaceId()).getMacAddress();

            dst = *dstMACAddr;

            LOG_DEBUG("Sending Pckt w/ Dst "
                        << destination.getAddressPart(7)
                        << " over " << dst.a4());
            return RR_SUCCESS;
        } else {
            LOG_WARN("Cld not rslv Nghbr");
            return RR_NO_NEIGHBOR;
        }
    }
}

#ifdef COMETOS_V6_RPL_SR
routeResult_t IpForward::findSourceRoute(
        const IPv6Datagram * datagram,
        Ieee802154MacAddress & src,
        Ieee802154MacAddress & dst) {

    IPv6Address destination = datagram->dst;
    const IPv6Route * sourceRoute = srt->doLongestPrefixMatch(destination);

    if (sourceRoute == NULL) {
            LOG_DEBUG("No Rt to Dst!");
            return RR_NO_ROUTE;
    }
    else {
        // We take the data from the source Routing Header
        FollowingHeader* fh = datagram->getNextHeader();
        while (fh != NULL && fh->getHeaderType() != IPv6RoutingHeader::HeaderNumber) {
            fh = fh->getNextHeader();
        }
        ASSERT(fh!=NULL);
        IPv6RoutingHeader * SRHeader = (IPv6RoutingHeader *)fh;
        int numberHops = SRHeader->getSegmentsLeft()+1;
        const uint8_t* addressData = SRHeader->getHData();
        IPv6Address hops[IP_MAXCONTENTS];
        int j=0;
        for (int index=0;index<numberHops;index++) {
            hops[index] = IPv6Address(&addressData[j]);
            j += IP6_BYTES;
        }

        // If there is still segments left in the header,
        // the datagram must go to the next hop
        if (SRHeader->getSegmentsLeft() > 1){
            //Intermediate Hops before reaching the destination
            LOG_DEBUG("Get MAC Addr");

            // We check which one is the next hop
            IPv6Address nh;
            IPv6Address sourceAddress = it->getInterface(sourceRoute->getInterfaceId()).getAddress(0);
            for (int i=0;i<numberHops;i++) {
                if (hops[i]==sourceAddress) {
                    nh = hops[i+1];
                }
            }

            const Ieee802154MacAddress* dstMACAddr = nd->resolveNeighbor(nh);

            if (dstMACAddr != NULL) {
                src = it->getInterface(sourceRoute->getInterfaceId()).getMacAddress();
                dst = *dstMACAddr;

                LOG_DEBUG("Sending Pckt w/ Dst "
                            << destination.getAddressPart(7)
                            << " over " << dst.a4());
                return RR_SUCCESS;
            } else {
                LOG_DEBUG("Cld not rslv Nghbr");
                return RR_NO_NEIGHBOR;
            }
            SRHeader->decrSegmentsLeft();
        }
        else if (SRHeader->getSegmentsLeft() == 1) {
            // The last segment is the direct route to the destination

            LOG_DEBUG("Get MAC Addr");
            const Ieee802154MacAddress* dstMACAddr = nd->resolveNeighbor(destination);

            if (dstMACAddr != NULL) {
                src = it->getInterface(sourceRoute->getInterfaceId()).getMacAddress();
                dst = *dstMACAddr;

                LOG_DEBUG("Sending Pckt to Dst "
                              << destination.getAddressPart(7));
                return RR_SUCCESS;
            } else {
                LOG_DEBUG("Cld not rslv Nghbr");
                return RR_NO_NEIGHBOR;
            }
        }
        else {
            return RR_NO_ROUTE;
        }
    }
}
#endif

void IpForward::forwardToUpper(ContentRequest *cRequest) {
    if (cRequest->content->getHeaderType() == ICMPv6Message::HeaderNumber) {
        toICMP.send(cRequest);
    } else if (cRequest->content->getHeaderType() == UDPPacket::HeaderNumber) {
        toUDP.send(cRequest);
    } else {
        toUnknown.send(cRequest);
    }
}

} // End of Namespace
