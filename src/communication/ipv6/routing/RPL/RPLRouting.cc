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
 * @author Fabian Krome, Martin Ringwelski, Andreas Weigel
 */


/*INCLUDES-------------------------------------------------------------------*/
#include "IPv6InterfaceTable.h"
#include "RPLRouting.h"
#include "RPLObjectiveFunction.h"
#include "RPLObjFunction0.h"
#include "RPLMRHOF.h"
#include "RemoteAccess.h"


namespace cometos_v6 {

Define_Module(RPLRouting);

/*
 * RPL_Instance
 */

RPLRouting::RPLRouting(const char * name) :
        TrickleModule(name,
                RPLCodec::getTrickleImin(DEFAULT_DIO_INTERVAL_MIN),
                DEFAULT_DIO_INTERVAL_DOUBLINGS,
                DEFAULT_DIO_REDUNDANCY_CONSTANT),
        storing(true),
        scheduleDAO(false),
        DODAGInstance(false, DEFAULT_MIN_ADV_RANK, DIS_SEND_INTERVAL, DEFAULT_GLOBAL_REPAIR),
        connected(false),
        poisonSubDAG(false),
        neverConnected(true),
        ICMPv6Layer(NULL),
        routingTable(NULL),
        pathSequence(0)
{
    //DEFAULT MACROS
    //DIO_Info
    //this->DODAGInstance.DIO_info.DODAGID = NULL;
    this->DODAGInstance.DIO_info.DTSN = 0;
#ifdef COMETOS_V6_RPL_SR
    sourceRoutingTable=NULL;
    if(storing){
        this->DODAGInstance.DIO_info.MOP = MOP_STORING_MULTICAST;
    }
    else {
        //If storing is set to false, the the mode is non-storing
        this->DODAGInstance.DIO_info.MOP = MOP_NON_STORING;
    }
#endif

    this->DODAGInstance.DIO_info.RPLinstanceID = RPL_DEFAULT_INSTANCE;
#ifdef RPL_GROUNDED
    this->DODAGInstance.DIO_info.grounded = true;
#else
    this->DODAGInstance.DIO_info.grounded = false;
#endif

#if defined(COMETOS_V6_RPL_MRHOF) && defined(COMETOS_V6_RPL_MRHOF_ETX)
    this->DODAGInstance.DIO_info.metricContainer.MCType = RPL_DODAG_MC_ETX;
#else
    this->DODAGInstance.DIO_info.metricContainer.MCType = RPL_DODAG_MC_HOPCOUNT;
#endif
    //Only 1 metric data
    this->DODAGInstance.DIO_info.metricContainer.length = 1;
    this->DODAGInstance.DIO_info.metricContainer.ResFlags = 0;
    this->DODAGInstance.DIO_info.metricContainer.P = false;
    this->DODAGInstance.DIO_info.metricContainer.C = false;
    this->DODAGInstance.DIO_info.metricContainer.O = false;
    this->DODAGInstance.DIO_info.metricContainer.R = true;
    this->DODAGInstance.DIO_info.metricContainer.AField = 0;
    this->DODAGInstance.DIO_info.metricContainer.PrecField = 0;
    this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
    this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
    this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;


    setMetric(1);



    this->DODAGInstance.DIO_info.preference = 0;
    this->DODAGInstance.DIO_info.rank = INFINITE_RANK;
    this->DODAGInstance.DIO_info.versionNumber = 0;


    //DEFINE
#ifdef RPL_AUTHENTICATION_ENABLE
    this->DODAGInstance.DIO_info.DAGConfig.authentificationEnable = true;
#else
    this->DODAGInstance.DIO_info.DAGConfig.authentificationEnable = false;
#endif

    this->DODAGInstance.DIO_info.DAGConfig.PCS = DEFAULT_PATH_CONTROL_SIZE;
    //DEFINE
    this->DODAGInstance.DIO_info.DAGConfig.intervalDoublings = DEFAULT_DIO_INTERVAL_DOUBLINGS;
    //DEFINE
    this->DODAGInstance.DIO_info.DAGConfig.intervalMin = DEFAULT_DIO_INTERVAL_MIN;
    //DEFINE
    this->DODAGInstance.DIO_info.DAGConfig.redundancyConst = DEFAULT_DIO_REDUNDANCY_CONSTANT;

    this->DODAGInstance.DIO_info.DAGConfig.maxRankIncrease = DEFAULT_MAX_RANK_INCREASE;

    this->DODAGInstance.DIO_info.DAGConfig.minHopRankIncrease = DEFAULT_MIN_HOP_RANK_INCREASE;

    //Setting of the Objective Function
    // OCP = RPL_OBJFUNC_OF0 -> Objective Function 0
    // OCP = RPL_OBJFUNC_MRHOF -> MRHOF (Minimum Rank with Hysteresis Objective Function)
#ifdef COMETOS_V6_RPL_MRHOF
    this->DODAGInstance.DIO_info.DAGConfig.OCP = RPL_OBJFUNC_MRHOF;
#else
    this->DODAGInstance.DIO_info.DAGConfig.OCP = RPL_OBJFUNC_OF0;
#endif

    this->DODAGInstance.DIO_info.DAGConfig.defaultLifetime = 5;
    this->DODAGInstance.DIO_info.DAGConfig.lifetimeUnit = 1;



    //defaults DAO
    //DAO_Information...
    DAO_Information.D = false;
    DAO_Information.DAOSequence = 0;
    DAO_Information.DAO_TransientInfoSize = 0;
    //DAO_Information.DODAGID = NULL;
    DAO_Information.RPLInstanceID = 0;
    DAO_Information.RPL_TargetSize = 0;
    DAO_Information.k = false;
}

RPLRouting::~RPLRouting(){
    cancel(&msg);
}

// initializes instance with
void RPLRouting::initialize(){

    remoteDeclare(&RPLRouting::printAllRoutes, "routes");


#ifdef OMNETPP
    // must be set before starting DIO Trickle timer
    this->DODAGInstance.DIO_info.DAGConfig.intervalMin = par("defaultDioIntervalMin");
    this->DODAGInstance.DIO_info.DAGConfig.intervalDoublings = par("defaultDioIntervalDoublings");
    this->DODAGInstance.DIO_info.DAGConfig.redundancyConst = par("defaultDioRedundancyConst");
    this->DODAGInstance.DIO_info.DAGConfig.authentificationEnable = par("authentificationEnable");
    this->DODAGInstance.DIO_info.DAGConfig.minHopRankIncrease = par("defaultDioMinHopRankIncrease");
    this->DODAGInstance.root = par("defaultDioRoot");
    this->DODAGInstance.DIO_info.grounded = par("grounded");
#else
    this->DODAGInstance.root = RPL_DEFAULT_DIO_ROOT;
#endif

    if (DODAGInstance.root){
        // root initialization and DIO Trickle start
        rootDODAG();
    }
    initModules();


    //Call of the selected Objective Function
    initObjFunc();
    DODAGInstance.neighborhood.init();

    if (this->DODAGInstance.DIO_info.grounded) {
        IPv6Address ground = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress();
        ground.setAddressPart(0,7);
        LOG_WARN("grndd, deflt Rt: " << ground.str().c_str());

        routingTable->updateDefaultRoute(&ground, INTERFACE_ID, 0);
    }

    // TODO for now, we deactivate DAOs and upward routes, because DIOs are not
    //      really working the way they should;
    //      additionally, shouldn't sending of DAOs depend on modes of operation
    //      and only be initialized when a suitable DODAG was joined by a node?
    //schedule(new cometos::Message, &RPLRouting::DAO_TimerTimeout,
    //            DODAGInstance.DIO_info.DAGConfig.defaultLifetime *
    //            DODAGInstance.DIO_info.DAGConfig.lifetimeUnit / 2
    //            + intrand(DODAGInstance.DIO_info.DAGConfig.defaultLifetime * DODAGInstance.DIO_info.DAGConfig.lifetimeUnit));
    schedule(new cometos::Message(), &RPLRouting::secondStageInitialize, 1);

    schedule(&msg, &RPLRouting::DAO_TimerDelay, DEFAULT_DAO_DELAY / 2 + intrand(DEFAULT_DAO_DELAY));
}

void RPLRouting::initModules(){
    LOG_INFO("getting ptrs");

    ICMPv6Layer = static_cast<ICMPv6 *>(getModule(ICMP_MODULE_NAME));
    ASSERT(ICMPv6Layer!=NULL);

    routingTable = static_cast<RPLRoutingTable *>(getModule(RPL_ROUTING_TABLE_MODULE_NAME));
    ASSERT(routingTable!=NULL);

#ifdef COMETOS_V6_RPL_SR
    sourceRoutingTable = static_cast<RPLSourceRoutingTable *>(getModule(SOURCE_ROUTING_TABLE_MODULE_NAME));
            ASSERT(sourceRoutingTable!=NULL);
#endif
}

void RPLRouting::secondStageInitialize(cometos::Message* msg) {
    delete msg;
    //    ICMPv6Layer->registerListener(this, &RPLRouting::handleICMPMessage, RPL_ICMP_MESSAGE_TYPE);
    LOG_INFO("reg listener");

    if(ICMPv6Layer->registerListener(this, RPL_ICMP_MESSAGE_TYPE)) {
        LOG_DEBUG("true");
    } else {
        LOG_ERROR("false");
    }
}

void RPLRouting::initObjFunc(){
    LOG_INFO("init OF");
    if (DODAGInstance.DIO_info.DAGConfig.OCP == RPL_OBJFUNC_OF0) {
           LOG_DEBUG("OF0");
           objectiveFunction = new RPLObjFunction0;
       }
    else if (DODAGInstance.DIO_info.DAGConfig.OCP == RPL_OBJFUNC_MRHOF) {
       objectiveFunction = new RPLMRHOF;
       LOG_DEBUG("MRHOF");
       objectiveFunction->curMinPathCost = INFINITE_RANK;
       if(DODAGInstance.root){
           objectiveFunction->curMinPathCost = DODAGInstance.DIO_info.rank;
       }
    }
    objectiveFunction->initialize(&DODAGInstance, SEQUENCE_WINDOW);
}


void RPLRouting::rootDODAG(){
    DODAGInstance.neighborhood.removeAllNeighbors();
    //TODO RoutingTable what to do with packages with no goal -> so far ignored !!!!

    ASSERT(getModule(INTERFACE_TABLE_MODULE_NAME)!=NULL);
    this->DODAGInstance.DIO_info.DODAGID = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress();

    //Node connected if it creates a tree
    connected = true;
    if(neverConnected){
        neverConnected = false;
    }

    DODAGInstance.DIO_info.versionNumber++;
    //TODO DEFAULT LIFETIME
    this->DODAGInstance.DIO_info.DAGConfig.defaultLifetime = 20;
    //DIO INO
    this->DODAGInstance.DIO_info.DTSN = ROOT_DEFAULT_DTSN;

    if (storing){
        this->DODAGInstance.DIO_info.MOP = MOP_STORING_MULTICAST;
    } else {
        this->DODAGInstance.DIO_info.MOP = MOP_NON_STORING;
    }

    this->DODAGInstance.DIO_info.RPLinstanceID = ROOT_DEFAULT_RPLinstanceID;
    this->DODAGInstance.DIO_info.grounded = ROOT_DEFAULT_grounded;

#if defined(COMETOS_V6_RPL_MRHOF) && defined(COMETOS_V6_RPL_MRHOF_ETX)
    this->DODAGInstance.DIO_info.metricContainer.MCType = RPL_DODAG_MC_ETX;
#else
    this->DODAGInstance.DIO_info.metricContainer.MCType = RPL_DODAG_MC_HOPCOUNT;
#endif
    //Only 1 metric data
    this->DODAGInstance.DIO_info.metricContainer.length = 1;
    this->DODAGInstance.DIO_info.metricContainer.ResFlags = 0;
    this->DODAGInstance.DIO_info.metricContainer.P = false;
    this->DODAGInstance.DIO_info.metricContainer.C = false;
    this->DODAGInstance.DIO_info.metricContainer.O = false;
    this->DODAGInstance.DIO_info.metricContainer.R = true;
    this->DODAGInstance.DIO_info.metricContainer.AField = 0;
    this->DODAGInstance.DIO_info.metricContainer.PrecField = 0;
    if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_HOPCOUNT) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_ETX) {
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_LQL) {
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;
    }
    else {
        //By default the metric is hops
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
    }
    setMetric(1);

    this->DODAGInstance.DIO_info.preference = ROOT_DEFAULT_preference;
    this->DODAGInstance.DIO_info.rank = ROOT_DEFAULT_rank;
    this->DODAGInstance.DIO_info.versionNumber = ROOT_DEFAULT_versionNumber;

    //DAO_Information clean
    DAO_Information.D = false;
    DAO_Information.DAOSequence = 0;
    DAO_Information.DAO_TransientInfoSize = 0;
    //DAO_Information.DODAGID = NULL;
    DAO_Information.RPLInstanceID = 0;
    DAO_Information.RPL_TargetSize = 0;
    DAO_Information.k = false;

    //TODO beaviour of nodes - instant neighbor discovery or top down approach
    //TODO i do not understand the previous todo

    //start trickle timer by resetting (starting with minimum interval)
    setTrickleModule(RPLCodec::getTrickleImin(this->DODAGInstance.DIO_info.DAGConfig.intervalMin),
                     this->DODAGInstance.DIO_info.DAGConfig.intervalDoublings,
                     this->DODAGInstance.DIO_info.DAGConfig.redundancyConst);
    reset_();

    //TODO possible global repair after x -> schedule increase of versionNumber
}

bool RPLRouting::addOrModifyEntry(IPv6Address *destPrefix, uint8_t lengthPrefix, const IPv6Address * nextHop, int metric){

#ifdef COMETOS_V6_RPL_SR

    // If root in non-storing mode, don't add any routes so the source route will take place
    if ((DODAGInstance.DIO_info.MOP == MOP_NON_STORING) && (DODAGInstance.root)){
        //Save the info in the routing table for non-storing mode
        return sourceRoutingTable->modifyRoute(destPrefix, lengthPrefix, nextHop, INTERFACE_ID, metric);
    }
    else if (DODAGInstance.DIO_info.MOP == MOP_NON_STORING) {
        if(sourceRoutingTable->doLongestPrefixMatch(*destPrefix) == NULL){
            sourceRoutingTable->addRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
            return true;
        }
        if(sourceRoutingTable->doLongestPrefixMatch(*destPrefix) == sourceRoutingTable->getDefaultRoute()){
            sourceRoutingTable->addRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
            return true;
        }else{
            IPv6Address oldNextHop = sourceRoutingTable->doLongestPrefixMatch(*destPrefix)->getNextHop();
            //change Route
            sourceRoutingTable->modifyRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
            //Equal
            if(oldNextHop == *nextHop){
                return true;
            }
            return false;
        }
    }
    else
#endif
    {
        const IPv6Route *routeSearch = routingTable->doLongestPrefixMatch(*destPrefix);

        //Routing Table handling - is return value default route or no default? -> add Route
        if(routeSearch == NULL){
           routingTable->addRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
           return true;
        }

        if(routeSearch == routingTable->getDefaultRoute()){
            routingTable->addRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
            return true;
        }else{
            IPv6Address oldNextHop = routeSearch->getNextHop();
            //change Route
            routingTable->modifyRoute(destPrefix, lengthPrefix ,nextHop, INTERFACE_ID, metric);
            //Equal
            if(oldNextHop == *nextHop){

                return true;
            }
            return false;
        }
    }

}


uint8_t RPLRouting::addNeighbor(uint8_t rank, uint16_t DODAGVersionNumber, DAG_MC metricValue, IPv6Address ip){
    uint8_t index = DODAGInstance.neighborhood.addNeighbor(rank, DODAGVersionNumber, metricValue, ip);

    if(index == FULL_NEIGHBOORHOOD){
        return FULL_NEIGHBOORHOOD;
    }
    if(index == SAME_IPADDRESS_IN_NEIGHBOORHOOD){
        return SAME_IPADDRESS_IN_NEIGHBOORHOOD;
    }


    checkIfParent( DODAGInstance.neighborhood.getNeighbor(index));
    return index;
}

void RPLRouting::checkIfParent(RPLNeighbor *neighbor){
    if((objectiveFunction->compareRank(DODAGInstance.DIO_info.rank, neighbor->getRank()))
            == RANK_BIGGER){
        neighbor->setIsParent(true);
    }else{
        neighbor->setIsParent(false);
    }
}


void RPLRouting::choosePrefParent(parentChangeCause cause){
    LOG_DEBUG("Choosing preferred parent");

    if(DODAGInstance.DIO_info.rank == ROOT_DEFAULT_rank ){
        LOG_DEBUG("is root rank");
        return;
    }

    //Reset DAO Trigger Sequence Number
    this->DODAGInstance.DIO_info.DTSN = 0;
    uint16_t oldVersion = DODAGInstance.DIO_info.versionNumber;
    IPv6Address oldIP(IPV6_MUTLICAST);
    RPLNeighbor *oldParent = DODAGInstance.neighborhood.getPrefParent();
    if(oldParent != NULL){
        oldIP = DODAGInstance.neighborhood.getPrefParent()->getIpAdress();
    }

    RPLNeighbor * newParent = NULL;

    if(cause == PREF_PARENT_LOST){
        //delete Lost parent
        //STRICT SOLUTION TO DELETE HERE
        LOG_INFO("PREF_PARENT_LOST ");
        DODAGInstance.neighborhood.removeNeighbor(DODAGInstance.neighborhood.getPrefParentIndex());
        cleanDAO();
    }

    if(cause == NEW_DODAG_VERSION){
        DODAGInstance.neighborhood.setPrefParent(NO_PREF_PARENT);
        LOG_INFO("NEW_DODAG_VERSION ");
    }

    //Neighborhood
    for(uint8_t i = 0; i < DODAGInstance.neighborhood.getSize(); i++){
        RPLNeighbor * candidate = DODAGInstance.neighborhood.getNeighbor(i);
        LOG_DEBUG("TEST PREF PARENT neighbor ip: "<< candidate->getIpAdress().str().c_str());
        //ERROR if NULL
        if(candidate == NULL){
            return;
        }

        //if there is still a pref parent ignore none-parent nodes
        if(DODAGInstance.neighborhood.getPrefParent()!=NULL){
            LOG_DEBUG("PrefParent still available: ");
            if(!candidate->getIsParent()){
                LOG_DEBUG("No parent so NO Candidate ");
                continue;
            }
        } else {
            LOG_DEBUG("PrefParent not available ");
            //only check neighbors higher in tree
            if(objectiveFunction->compareRank(candidate->getRank(), DODAGInstance.DIO_info.rank) != RANK_SMALLER){
                LOG_DEBUG("Rank("<< candidate->getRank()<<") bigger than old rank("<< DODAGInstance.DIO_info.rank<<") so NO candidate ");
                continue;
            }

        }

        //Select a newParent if none so far
        if(!newParent){
            LOG_DEBUG("potential new parent " );
            newParent = candidate;
        //Check if Version more fitting to node (same Version preferred, if not possible highest Version)
        } else if(newParent->getVersion() > oldVersion){
            LOG_DEBUG("potential new parent with newer version " );
            if((candidate->getVersion() == oldVersion)||
               (candidate->getVersion() > newParent->getVersion())){
                newParent = candidate;
            }
        //with same Version an objective function decides
        } else if(newParent->getVersion() == candidate->getVersion()){
            LOG_DEBUG("potential new parent with same version " );
            newParent = objectiveFunction->bestParent(newParent, candidate);
        }
    }

    if(DODAGInstance.neighborhood.setPrefParent(newParent)) {
        LOG_DEBUG("SUCCESS in setting PrefParent");
    } else {
        LOG_DEBUG("FAILURE in setting PrefParent");
    }
    //RPLNeighbor* prefPar = DODAGInstance.neighborhood.getPrefParent();


    if(newParent){
#ifdef ENABLE_LOGGING
        IPv6Address prefParent = DODAGInstance.neighborhood.getPrefParent()->getIpAdress();
        LOG_INFO("New Pref. Parent:"<<prefParent.str().c_str());
#endif
        LOG_DEBUG("New Parent!! :)");
        DODAGInstance.DIO_info.versionNumber = newParent->getVersion();


        //in Case of changed Version or parent - report change
        if ((DODAGInstance.DIO_info.versionNumber != oldVersion) ||
           (newParent != oldParent)){
            //reset Trickle timer due to inconsistency
            //LOG_DEBUG("Inconsistency 1");
            //TODO trickle
            reset_();
            //LOG_DEBUG("Inconsistency 4");
        } else {
            if (newParent->getIpAdress() != oldIP) {
                //reset Trickle timer due to inconsistency
                //LOG_DEBUG("Inconsistency 2");
                //TODO trickle
                reset_();
                //LOG_DEBUG("Inconsistency 3");
            }
        }
    }

    //Security reset of subtree
    if (newParent != NULL) {
        if((newParent != oldParent)||((newParent == oldParent) && (newParent->getIpAdress() != oldIP))) {
            poisonSubDAG = true;
        }
    }

    // connected if newParent found
    bool wasConnected = connected;
    connected = (newParent != NULL);

    //first Connection establisher?
    if(neverConnected && connected){
        neverConnected = false;
    }

    //Only do when MRHOF is enabled
    if(DODAGInstance.DIO_info.DAGConfig.OCP == RPL_OBJFUNC_MRHOF) {
        if (!connected) {
            objectiveFunction->resetCurMinPathCost();
        }
    }

    //if not connected rank is infinite else Calculate new rank
    if(!connected && wasConnected){
        LOG_DEBUG("was connected and now is not");
        DODAGInstance.DIO_info.rank = INFINITE_RANK;
        if (storing) {
            routingTable->deleteDefaultRoute();
        }
#ifdef COMETOS_V6_RPL_SR
        else {
            sourceRoutingTable->deleteDefaultRoute();
        }
#endif
        emptyDAO();
        poisonSubDAG = true;

    } else if(connected) {

        uint16_t oldRank = DODAGInstance.DIO_info.rank;
        objectiveFunction->calculateRank();
        LOG_DEBUG("is connected with Rank: " << DODAGInstance.DIO_info.rank<< " old Rank was: "<< oldRank);

        if(oldRank < DODAGInstance.DIO_info.rank){
            emptyDAO();
            poisonSubDAG = true;
        } else {
            if((newParent == oldParent) && (newParent->getIpAdress() == oldIP)){
                LOG_DEBUG("OLD == NEW");
                return;
            }
        }

#ifdef COMETOS_V6_RPL_SR
        if (DODAGInstance.DIO_info.MOP == MOP_NON_STORING){
            sourceRoutingTable->updateDefaultRoute(&DODAGInstance.neighborhood.getPrefParent()->getIpAdress(), INTERFACE_ID, DODAGInstance.neighborhood.getPrefParent()->getMetric().metric.calculateMetricValue(DODAGInstance.DIO_info.metricContainer.MCType));
            sourceRoutingTable->deleteRoute(DODAGInstance.neighborhood.getPrefParent()->getIpAdress(), DAO_Information);
        }
        else
#endif
        {
            routingTable->updateDefaultRoute(&DODAGInstance.neighborhood.getPrefParent()->getIpAdress(), INTERFACE_ID, DODAGInstance.neighborhood.getPrefParent()->getMetric().metric.calculateMetricValue(DODAGInstance.DIO_info.metricContainer.MCType));
            routingTable->deleteRoute(DODAGInstance.neighborhood.getPrefParent()->getIpAdress(), DAO_Information);
        }

        IPv6Address prefParent = DODAGInstance.neighborhood.getPrefParent()->getIpAdress();

        LOG_DEBUG("New Pref. Parent:"<<prefParent.str().c_str());
        //Update DAO
        // DAO_Object tempParentPrefDAO_info;


        RPL_target tempRPL_t(128,
                ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress(),
                0);
        //not sure about the newParent->getIpAdress()

        LOG_DEBUG("expireTime = "<< DODAGInstance.neighborhood.getPrefParent()->getExpireTime());

        DAO_TransientInfo tempDAO_tranInfo(false, 0, pathSequence++,
                DODAGInstance.neighborhood.getPrefParent()->getExpireTime(),
                DODAGInstance.neighborhood.getPrefParent()->getIpAdress());

        uint8_t currentIndex = DAO_Information.findTarget(tempRPL_t.target);
        Temp_Pref_Parent_Object tempDAO(tempRPL_t, tempDAO_tranInfo);


        if(DAO_Information.addOrModifyTarget(&tempDAO, currentIndex)){
            LOG_DEBUG("DAO Scheduled parent changed");
            scheduleDAO = true;
        }
    }
    updateParentSet();
}

//Update of Parents in neighborhood and inform if preferred parent still in set
bool RPLRouting::updateParentSet(){

    LOG_DEBUG("Update Parent set ");

    bool lostPrefParent = false;
    bool isParent = false;

    LOG_DEBUG("neighborhood size = " << (int) DODAGInstance.neighborhood.getSize());
    for(uint8_t i = 0; i < DODAGInstance.neighborhood.getSize(); i++){
        IPv6Address neighborAdress = DODAGInstance.neighborhood.getNeighbor(i)->getIpAdress();
        LOG_DEBUG("Update Parent set neighbor ip: "<<neighborAdress.str().c_str());
        //Is the node of infinite rank
        if(DODAGInstance.DIO_info.rank == INFINITE_RANK){
            LOG_DEBUG("Infinite Rank  ");
            isParent = true;
        } else {

            objectiveFunction->compareRank(DODAGInstance.neighborhood.getNeighbor(i)->getRank(), DODAGInstance.DIO_info.rank);

            LOG_DEBUG("Not infinite Rank ");
            //parent if same version, not infinite rank, bigger rank given by objective function
            isParent = ((DODAGInstance.DIO_info.versionNumber == DODAGInstance.neighborhood.getNeighbor(i)->getVersion())&
                    ((objectiveFunction->compareRank(DODAGInstance.neighborhood.getNeighbor(i)->getRank(), DODAGInstance.DIO_info.rank) == RANK_SMALLER)));
            LOG_DEBUG("neighbor: "<< i<< " is Parent: "<< (int) isParent);
        }

        if(DODAGInstance.neighborhood.getNeighbor(i)->getIsParent() != isParent){
            //LOG_DEBUG("Change in Parentbeing ");
            //change parentstatus if needed
            DODAGInstance.neighborhood.getNeighbor(i)->setIsParent(isParent);
            //check if preferred parent still parent
            if((DODAGInstance.neighborhood.getPrefParentIndex() == i) && (!isParent)){
                lostPrefParent = true;
            }
        }
    }
    LOG_DEBUG("Update Parent set finished: " << lostPrefParent);
    return lostPrefParent;
}


void RPLRouting::icmpMessageReceived(const IPv6Address &src, const IPv6Address & dst, uint8_t type, uint8_t code, const uint16_t additionalValues[2] , const uint8_t *data, uint16_t length) {
    ENTER_METHOD_SILENT();
    LOG_DEBUG("Msg rcvd; T: " << (int)code << " L: " << (int) length);

    if(type != RPL_ICMP_MESSAGE_TYPE){
        ASSERT(type == RPL_ICMP_MESSAGE_TYPE);
        return;
    }

    if(length > MAX_LENGTH-4){
        LOG_DEBUG("rcvd Msg too long, discarded");
        return;
    }
    memcpy(messageReceivedBuffer, (uint8_t*)additionalValues, 4);
    memcpy(messageReceivedBuffer + 4, data, length);

    if(code == DIS_TYPE){
        handleDIS(messageReceivedBuffer, length + 4);
        return;
    }

    if(code == DIO_TYPE){
        handleDIO(src, messageReceivedBuffer, length + 4);
        return;
    }

    if(code == DAO_TYPE){
        handleDAO(src, messageReceivedBuffer, length + 4);
        return;
    }

    //TODO If needed
    if(code == DAO_ACK_TYPE){
        ASSERT(code != DAO_ACK_TYPE);
        return;
    }
    else {
        LOG_WARN("unknown msg code");
    }
}

void RPLRouting::handleDIO(const IPv6Address &src, const uint8_t *data, uint16_t length){

    IPv6Address ownIP = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getLocalAddress();
    if(ownIP == src) {
        LOG_DEBUG("DIO from myself, ignoring ");
        return;
    }
    DIO_Object currentDIO_info;

    LOG_INFO("DIO from: " << src.str().c_str());

    //For stats
//    RPL_SCALAR_INC(numDIO);
//    stats.sizeDIO = length;

    RPLCodec::decodeDIO(currentDIO_info, data, length);

    //Unknown neighbor?
    uint8_t neighborIndex = DODAGInstance.neighborhood.findNeighborIndex(src);

    //Refresh neighbor live
    if(neighborIndex != NOT_A_NEIGHBOR){
        LOG_DEBUG("Capture Neighbor");
        DODAGInstance.neighborhood.refreshNeighbor(src);
    } else {
        LOG_DEBUG("DIO from unknown neighbor");
    }

    //Is node not already in a DODAG
    if(!connected){
        //no infinite rank nodes
        LOG_DEBUG("not connected with RANK = "<<currentDIO_info.rank);
        if(currentDIO_info.rank == INFINITE_RANK){
            LOG_DEBUG("infiniteRank DIO - not connected ");
            return;
        }
        //take information of
        DODAGInstance.DIO_info = currentDIO_info;
        DAG_ConfigUpdate();

        updateMetric(1);

        DODAGInstance.DIO_info.rank = INFINITE_RANK;
        if(neighborIndex != NOT_A_NEIGHBOR){
            DODAGInstance.neighborhood.removeNeighbor(neighborIndex);
            cleanDAO();
        }
        uint8_t size = DODAGInstance.neighborhood.addNeighbor(currentDIO_info.rank, currentDIO_info.versionNumber, currentDIO_info.metricContainer, src);
        if(size == FULL_NEIGHBOORHOOD){
            //if full neigborhood delete neighbor with worst -TODO so far just any neighbor
            DODAGInstance.neighborhood.removeNeighbor(0);
            cleanDAO();
        }
        //initialize new parent (has to be parent as it has a rank < infinite rank)
        //LOG_DEBUG("Choose Pref. Parent ");

        choosePrefParent(NO_CAUSE);

        LOG_DEBUG("Pref. Parent found?:" << (int) connected );
        //start trickle timer, as now valuable information in DIO
        if(connected){
            // start sending DIOs right away -- use reset_ instead of start_ to
            // start with minimum interval
            reset_();
        }
        return;
    }


    //As connected check if DIO of same DODAG and with same InstanceId
    if((DODAGInstance.DIO_info.DODAGID != currentDIO_info.DODAGID)
            ||(DODAGInstance.DIO_info.RPLinstanceID != currentDIO_info.RPLinstanceID)){
        //else return
        return;
    }

    //peer in neighborhood
    if(neighborIndex == NOT_A_NEIGHBOR){
        LOG_DEBUG("not a neighbor");

        //add it

        DODAGInstance.neighborhood.addNeighbor(currentDIO_info.rank, currentDIO_info.versionNumber, currentDIO_info.metricContainer, src);
        //uint8_t size = DODAGInstance.neighborhood.addNeighbor(currentDIO_info.rank, currentDIO_info.versionNumber, currentDIO_info.metricContainer, src);
        //if(size == FULL_NEIGHBOORHOOD){
            //if full neighborhood delete neighbor with worst -TODO so far just any neighbor
            //DODAGInstance.neighborhood.removeNeighbor(0);
        //}
        updateParentSet();
        choosePrefParent(NO_CAUSE);
    } else {

        LOG_DEBUG("already neighbor");

        RPLNeighbor * neighbor = DODAGInstance.neighborhood.getNeighbor(neighborIndex);

        bool rankChange = false;
        bool versionChange = false;
        //check for version change
        versionChange = (neighbor->getVersion() != currentDIO_info.versionNumber);
        uint16_t neighborRank = neighbor->getRank();
        uint16_t DIORank = currentDIO_info.rank;
        rankChange = objectiveFunction->compareRank(neighborRank, DIORank) != RANK_EQUAL;
        neighbor->setRank(currentDIO_info.rank);
        neighbor->setVersion(currentDIO_info.versionNumber);
        //if neighbor is a parent and version changed neighbor not parent anymore
        if(versionChange && neighbor->getIsParent()){
            neighbor->setIsParent(false);
            //if neighbor was preferred parent new pref. parent has to be found
            if(neighbor == DODAGInstance.neighborhood.getPrefParent()){
                choosePrefParent(NEW_DODAG_VERSION);
            }
        }
        //Has a Rank changed but not the version
        if(!versionChange && rankChange){
            //if neighbor is preferred parent
            if(neighbor == DODAGInstance.neighborhood.getPrefParent()){
                //if neighbor has infinite rank choose ne prefParent (parent counts as lost)
                if(neighbor->getRank() == INFINITE_RANK){
                    choosePrefParent(PREF_PARENT_LOST);
                //if not infinite rank, own rank has to be adjusted
                } else {
                    objectiveFunction->calculateRank();
                    //changed to infinite Rank?
                    if(DODAGInstance.DIO_info.rank == INFINITE_RANK){
                        //parent lost
                        choosePrefParent(PREF_PARENT_LOST);
                    }else {
                        //Rank change, needs to check parents
                        updateParentSet();
                        //DAO Schedule, compares if the current DTSN has been increased
                        if(objectiveFunction->compareSequence(currentDIO_info.DTSN, DODAGInstance.DIO_info.DTSN) == SEQUENCE_BIGGER){
                            LOG_DEBUG("DAO needed for transmission!");
                            //emptyDAO(); In the RFC only says that a DAO should be sent
                            scheduleDAO = true;
                            if (currentDIO_info.MOP == MOP_NON_STORING){
                                //The node must increment its own DTSN
                                currentDIO_info.DTSN++;
                            }
                        }
                        scheduleDAO = true; // FIXME: This is just a test.
                        DODAGInstance.DIO_info.DTSN = currentDIO_info.DTSN;
                    }
                }
                //not pref. parent
            } else{
                //rank changed of node, is it now a parent, is it not a parent anymore
                if(!DODAGInstance.root){
                    neighbor->setIsParent(objectiveFunction->compareRank(neighbor->getRank(), DODAGInstance.DIO_info.rank) == RANK_SMALLER);
                }
                choosePrefParent(NO_CAUSE);
            }
        }

        //if neighbor is preferred parent
        if(neighbor == DODAGInstance.neighborhood.getPrefParent()){
            if(objectiveFunction->compareSequence(currentDIO_info.DTSN, DODAGInstance.DIO_info.DTSN) == SEQUENCE_BIGGER){
                LOG_DEBUG("DAO needed for transmission!");
                //emptyDAO(); In the RFC only says that a DAO should be sent
                scheduleDAO = true;
            }
            scheduleDAO = true; // FIXME: This is just a test.
            DODAGInstance.DIO_info.DTSN = currentDIO_info.DTSN;
        }
    }

}

void RPLRouting::DAG_ConfigUpdate(){
    //Set trickle Timer values!
    setTrickleModule(
        RPLCodec::getTrickleImin(DODAGInstance.DIO_info.DAGConfig.intervalMin),
        DODAGInstance.DIO_info.DAGConfig.intervalDoublings,
        DODAGInstance.DIO_info.DAGConfig.redundancyConst);

    //TODO choose objective Function -> it's done in the initialize

    //Default Lifetime
    DODAGInstance.neighborhood.setDefaultLifetime(DODAGInstance.DIO_info.DAGConfig.defaultLifetime);

    //Lifetime unit
    DODAGInstance.neighborhood.setTimeMultiplier(DODAGInstance.DIO_info.DAGConfig.lifetimeUnit);
}

void RPLRouting::transmit(){
    LOG_DEBUG("TrickleTimer fires");

    if(poisonSubDAG){
        uint16_t oldRank = DODAGInstance.DIO_info.rank;

        LOG_INFO("Poison subtree, oldRank=" << oldRank);
        //poison subtree with infinite Rank
        DODAGInstance.DIO_info.rank = INFINITE_RANK;
        sendDIO();
        DODAGInstance.DIO_info.rank = oldRank;

        poisonSubDAG = false;
        return;
    }

    if(connected){
        LOG_DEBUG("connected, sending DIO");
        sendDIO();
    } else {
        LOG_INFO("not connected, sending DIS");
        sendDIS();
    }

}
void RPLRouting::handleDIS(const uint8_t *data, uint16_t length){
    //TODO  Solicited Information request (needed here?)
    //TODO decodeDIS()

    //reset Trickle for DIO as requested
    stop_();
    reset_();
    start_();
    LOG_WARN("Got DIS");
    sendDIO();
    //
}

void RPLRouting::handleDAO(const IPv6Address &src, const uint8_t *data, uint16_t length){
    LOG_DEBUG("      DAO received from: " << src.str().c_str());
//    DAO_Object currentDAO_info;
    uint8_t hops;

    if(!connected){
        LOG_WARN("NOT CONNECTED? ");
        return;
    }

    if(DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(src)) == NULL){
        //Ignore DAO from node not in neighbor set
        LOG_WARN("DAO from node not in neighbor set");
        return;
    }
    DODAGInstance.neighborhood.refreshNeighbor(src);
    //else refresh Timeout

    uint8_t currentIndex;

    //For stats
//    RPL_SCALAR_INC(numDAO);
//    stats.sizeDAO = length;

    //Decoding
    RPLCodec::decodeDAO(currentDAO_info, data, length);

    //Check if DAO_Sequence greater
    if(objectiveFunction->compareSequence(DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(src))->getDAOSequence(), currentDAO_info.DAOSequence == SEQUENCE_BIGGER)){
        DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(src))->setDAOSequence(currentDAO_info.DAOSequence);
    } else {
        //DAO without newer sequence number, so ignore it
        LOG_WARN("DAO without newer sequence number");
       // return;
    }

    //In non storing mode only root supposed to receive DAO
    if((DODAGInstance.DIO_info.MOP == MOP_NON_STORING) && (!DODAGInstance.root)){
        LOG_WARN("NON STORING: A non-root receive a DAO. They are not supposed to");
        return;
    }

    //Processing DAO
    bool modified = false;

    //Routing input
    for(uint8_t i = 0; i < currentDAO_info.RPL_TargetSize; i++){
        if(storing){
            currentIndex = DAO_Information.findTarget(currentDAO_info.RPL_Targets[i].target);
            //Loop check
            if(currentDAO_info.RPL_Targets[i].target == ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress()){
                //node is in sub tree of node
                poisonSubDAG = true;
                //TODO - loop avoidance
            }
            //Check for no path entry - loss of reachability
            if(currentDAO_info.DAO_TransientInfos[currentDAO_info.RPL_Targets[i].transientInfoIndex].pathLifetime == NO_PATH_LIFETIME){
                //TODO new target information? - already established route??
                //Remove the last Downward route
                if (currentIndex == MAX_RPL_TARGETS) {
                    LOG_DEBUG("No target found");
                    IPv6Address target = currentDAO_info.RPL_Targets[i].target;
                    LOG_WARN("Deleting " << target.str().c_str());
                    modified = true;
                    deleteEntry(currentDAO_info.RPL_Targets[i].target);
                }
                else {
                    modified = modified | DAO_Information.deleteTarget(currentIndex);
                    deleteEntry(currentDAO_info.RPL_Targets[i].target);
                    //Update own DAO info
                }
            }else{
                //TODO new target information? - already established route??
                //Update own DAO info
                modified = modified | DAO_Information.addOrModifyTarget(&currentDAO_info, currentIndex, i);
                hops = DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(src))->getMetric().metric.calculateMetricValue(DODAGInstance.DIO_info.metricContainer.MCType);
                modified = modified | addOrModifyEntry(&currentDAO_info.RPL_Targets[i].target, IP6_BYTES * 8, &src, hops);
            }
        }
#ifdef COMETOS_V6_RPL_SR
        else {
            currentIndex = DAO_Information.findTarget(currentDAO_info.RPL_Targets[i].target);
            //Loop check
            if(currentDAO_info.RPL_Targets[i].target == ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress()){
                //node is in sub tree of node
                poisonSubDAG = true;
                //TODO - loop avoidance
            }
            //Check for no path entry - loss of reachability
            if(currentDAO_info.DAO_TransientInfos[currentDAO_info.RPL_Targets[i].transientInfoIndex].pathLifetime == NO_PATH_LIFETIME){
                //TODO new target information? - already established route??
                //Remove the last Downward route
                if (currentIndex == MAX_RPL_TARGETS) {
                    LOG_DEBUG("No target found");
                    IPv6Address target = currentDAO_info.RPL_Targets[i].target;
                    LOG_DEBUG("Deleting " << target.str().c_str());
                    modified = true;
                    deleteEntry(currentDAO_info.RPL_Targets[i].target);
                }
                else {
                    modified = modified | DAO_Information.deleteTarget(currentIndex);
                    deleteEntry(currentDAO_info.RPL_Targets[i].target);
                    //Update own DAO info
                }
            }else{
                //TODO new target information? - already established route??
                //Update own DAO info
                modified = modified | DAO_Information.addOrModifyTarget(&currentDAO_info, currentIndex, i);
                hops = DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(src))->getMetric().metric.calculateMetricValue(DODAGInstance.DIO_info.metricContainer.MCType);

                //If it's the root, then the next hop is the destination (child)
                modified = modified | addOrModifyEntry(&currentDAO_info.RPL_Targets[i].target, IP6_BYTES * 8, &currentDAO_info.RPL_Targets[i].target, hops);
                if (modified) {
                    //The route should send the DAO to the destination
                    scheduleDAO = true;
                }
            }
        }
#endif

    }

    //DEBUG DAO
    //Neighbors
    for(uint8_t j = 0; j < DAO_Information.RPL_TargetSize; j++){
        IPv6Address target = DAO_Information.RPL_Targets[j].target;
        LOG_DEBUG("Neighbor Nr: " << (int)j << " " << "Ip: " << target.str().c_str());
        if (storing) {
            if (routingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[j].target) != NULL) {
                IPv6Address nextHop = routingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[j].target)->getNextHop();
                LOG_DEBUG("Next hop: " << nextHop.str().c_str());
            } else {
                LOG_WARN("No Route to target " << target.str().c_str());
            }
        }
#ifdef COMETOS_V6_RPL_SR
        else {
            if (sourceRoutingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[j].target) != NULL) {
                IPv6Address nextHop = sourceRoutingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[j].target)->getNextHop();
                LOG_DEBUG("Next hop: " << << nextHop.str().c_str());
            } else {
                LOG_WARN("No Route to target " << target.str().c_str());
            }
        }
#endif
    }

    //Schedule own DAO if routing information changed (not root!)
    if(modified & !DODAGInstance.root){
        //TODO ONLY PREF PARENT IST DAO PARENT! -> More DAO parents not implemented
        //DELAY and Send after next timer

        //Shouldn't it call the DelayDao timer?
        scheduleDAO = true;
    }
    //TODO DAO ACK?
}

//add/change Routing entry
//delete Entry
bool RPLRouting::deleteEntry(IPv6Address ip){
    LOG_WARN("Deleting Route " << cometos::hex << ip.getAddressPart(7) << cometos::dec);
#ifdef COMETOS_V6_RPL_SR
    // If root in non-storing mode, don't delete any routes so the source route will take place
    if ((DODAGInstance.DIO_info.MOP == MOP_NON_STORING) && (DODAGInstance.root)){
        //Update the info in the routing table for non-storing mode
        return sourceRoutingTable->deleteRoute(ip, DAO_Information);
    }
    else
#endif
    {
        return routingTable->deleteRoute(ip, DAO_Information);
    }
}

void RPLRouting::sendDIS(){
    IPv6Address multicast(IPV6_MUTLICAST);
    uint16_t first4Bytes[2];
    first4Bytes[0] = 0;
    first4Bytes[1] = 0;
    if(ICMPv6Layer->sendMessage(multicast,
                   RPL_ICMP_MESSAGE_TYPE,
                   DIS_TYPE,
                   first4Bytes)){
            //worked
               LOG_INFO("DIS Sending Successful ");
        }else{
            LOG_ERROR("DIS Sending failed ");
        }
}


void RPLRouting::sendDIO(){
    IPv6Address source = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getLocalAddress(); //.getGlobalAddress()
    LOG_DEBUG("Sending DIO from: " <<source.str().c_str());
    LOG_DEBUG("to: Multicast" << " rank: " << DODAGInstance.DIO_info.rank);
    //TODO create Buffer field direct acces?
    //TODO Add pads
    bool pad1 = false;
    uint8_t padN = 0;
    bool DAG_MetricContainer = true;
    bool routingInfo = false;
    bool DODAG_Config = true;
    bool prefixInfo = false;

    uint16_t length = RPLCodec::encodeDIO(&DIO_Buffer[0], DODAGInstance.DIO_info, pad1, padN, DAG_MetricContainer, routingInfo, DODAG_Config, prefixInfo);

    ASSERT(length < MAX_LENGTH);

    //TEST TEST encode and decode
//    DIO_Object currentDIO_info;
//    RPLCodec::decodeDIO(currentDIO_info, &DIO_Buffer[0], length);
//    LOG_DEBUG("DIO DECODE and compare: ");
//    bool test = true;
//    test = test & (DODAGInstance.DIO_info.DODAGID == currentDIO_info.DODAGID);
//    LOG_DEBUG("test DODAGID" << test );
//    test = 1 & (DODAGInstance.DIO_info.DTSN == currentDIO_info.DTSN);
//    LOG_DEBUG("test DTSN" << test );
//    test = 1 & (DODAGInstance.DIO_info.MOP == currentDIO_info.MOP);
//    LOG_DEBUG("test MOP" << test );
//    test = 1 & (DODAGInstance.DIO_info.RPLinstanceID == currentDIO_info.RPLinstanceID);
//    LOG_DEBUG("test RPLInstanceID" << test );
//    test = 1 & (DODAGInstance.DIO_info.grounded == currentDIO_info.grounded);
//    LOG_DEBUG("test grounded" << test );
//    test = 1 & (DODAGInstance.DIO_info.preference == currentDIO_info.preference);
//    LOG_DEBUG("test preference" << test );
//    test = 1 & (DODAGInstance.DIO_info.rank == currentDIO_info.rank);
//    LOG_DEBUG("test rank" << test );
//    test = 1 & (DODAGInstance.DIO_info.versionNumber == currentDIO_info.versionNumber);
//
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.OCP == currentDIO_info.DAGConfig.OCP);
//    LOG_DEBUG("test OCP" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.OCP << " compared to "<< currentDIO_info.DAGConfig.OCP);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.PCS == currentDIO_info.DAGConfig.PCS);
//    LOG_DEBUG("test PCS" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.PCS << " compared to "<< currentDIO_info.DAGConfig.PCS);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.authentificationEnable == currentDIO_info.DAGConfig.authentificationEnable);
//    LOG_DEBUG("test authentificationEnable" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.authentificationEnable << " compared to "<< currentDIO_info.DAGConfig.authentificationEnable);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.defaultLifetime == currentDIO_info.DAGConfig.defaultLifetime);
//    LOG_DEBUG("test defaultLifetime" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.defaultLifetime << " compared to "<< currentDIO_info.DAGConfig.defaultLifetime);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.intervalDoublings == currentDIO_info.DAGConfig.intervalDoublings);
//    LOG_DEBUG("test intervalDoublings" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.intervalDoublings << " compared to "<< currentDIO_info.DAGConfig.intervalDoublings);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.intervalMin == currentDIO_info.DAGConfig.intervalMin);
//    LOG_DEBUG("test intervalMin" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.intervalMin << " compared to "<< currentDIO_info.DAGConfig.intervalMin);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.lifetimeUnit == currentDIO_info.DAGConfig.lifetimeUnit);
//    LOG_DEBUG("test lifetimeUnit" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.lifetimeUnit << " compared to "<< currentDIO_info.DAGConfig.lifetimeUnit);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.maxRankIncrease == currentDIO_info.DAGConfig.maxRankIncrease);
//    LOG_DEBUG("test maxRankIncrease" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.maxRankIncrease << " compared to "<< currentDIO_info.DAGConfig.maxRankIncrease);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.minHopRankIncrease == currentDIO_info.DAGConfig.minHopRankIncrease);
//    LOG_DEBUG("test minHopRankIncrease" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.minHopRankIncrease << " compared to "<< currentDIO_info.DAGConfig.minHopRankIncrease);
//    test = 1 & (DODAGInstance.DIO_info.DAGConfig.redundancyConst == currentDIO_info.DAGConfig.redundancyConst);
//    LOG_DEBUG("test redundancyCost" << test << " Original " << DODAGInstance.DIO_info.DAGConfig.redundancyConst << " compared to "<< currentDIO_info.DAGConfig.redundancyConst);
//
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.length == currentDIO_info.metricContainer.length);
//    LOG_DEBUG("test length" << test << " Original " << DODAGInstance.DIO_info.metricContainer.length << " compared to "<< currentDIO_info.metricContainer.length);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.MCType == currentDIO_info.metricContainer.MCType);
//    LOG_DEBUG("test MCType" << test << " Original " << DODAGInstance.DIO_info.metricContainer.MCType << " compared to "<< currentDIO_info.metricContainer.MCType);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.ResFlags == currentDIO_info.metricContainer.ResFlags);
//    LOG_DEBUG("test ResFlags" << test << " Original " << DODAGInstance.DIO_info.metricContainer.ResFlags << " compared to "<< currentDIO_info.metricContainer.ResFlags);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.P == currentDIO_info.metricContainer.P);
//    LOG_DEBUG("test P" << test << " Original " << DODAGInstance.DIO_info.metricContainer.P << " compared to "<< currentDIO_info.metricContainer.P);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.C == currentDIO_info.metricContainer.C);
//    LOG_DEBUG("test C" << test << " Original " << DODAGInstance.DIO_info.metricContainer.C << " compared to "<< currentDIO_info.metricContainer.C);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.O == currentDIO_info.metricContainer.O);
//    LOG_DEBUG("test O" << test << " Original " << DODAGInstance.DIO_info.metricContainer.O << " compared to "<< currentDIO_info.metricContainer.O);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.R == currentDIO_info.metricContainer.R);
//    LOG_DEBUG("test R" << test << " Original " << DODAGInstance.DIO_info.metricContainer.R << " compared to "<< currentDIO_info.metricContainer.R);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.AField == currentDIO_info.metricContainer.AField);
//    LOG_DEBUG("test AField" << test << " Original " << DODAGInstance.DIO_info.metricContainer.AField << " compared to "<< currentDIO_info.metricContainer.AField);
//    test = 1 & (DODAGInstance.DIO_info.metricContainer.metric.hops == currentDIO_info.metricContainer.metric.hops);
//    LOG_DEBUG("test hops" << test << " Original " << DODAGInstance.DIO_info.metricContainer.metric.hops << " compared to "<< currentDIO_info.metricContainer.metric.hops);
    //LOG_DEBUG("DOGAG - info and configuration test " << test );
    //TEST TEST

    uint16_t first4Bytes[2];
    first4Bytes[0] = DIO_Buffer[0] + (DIO_Buffer[1] << 8);
    first4Bytes[1] = DIO_Buffer[2] + (DIO_Buffer[3] << 8);

    IPv6Address multicast(IPV6_MUTLICAST);
  //TEST TEST

    //IPv6Address multicast = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getGlobalAddress();;
    //multicast.setAddressPart(0x21fd, 7);
    //LOG_DEBUG("Send Address: " << multicast.str());
    //routingTable->addRoute(&multicast, IP6_BYTES ,&multicast, INTERFACE_ID, 1);
//TEST TEST
    if(ICMPv6Layer->sendMessage(multicast,
               RPL_ICMP_MESSAGE_TYPE,
               DIO_TYPE,
               first4Bytes,
               &DIO_Buffer[4],
               length - 4)){
        //worked
           LOG_INFO("DIO Sending Successful ");
    }else{
        LOG_ERROR("DIO Sending failed ");
    }
}

void RPLRouting::sendDAO(){
    //SECURITY MEASSURE
    if(DODAGInstance.root){ // TODO: There was a "&& storing", why?
        //LOG_DEBUG("I am the root node and it is storing mode");
        return;
    }

    //TODO create Buffer field direct access?
    //TODO pads in encoding ?
    bool K = false;
    bool D = false;
    DAO_Information.DAOSequence++;
    bool pad1 = false;
    uint8_t padN = 0;

    uint16_t length = RPLCodec::encodeDAO(DODAGInstance.DIO_info, DAO_Information, &DAO_Buffer[0], K, D, pad1, padN);

    if (length == 0) {
        //The DAO couldn't be encoded
        LOG_DEBUG("DAO couldn't be encoded");
        return;
    }

    uint16_t first4Bytes[2];
    first4Bytes[0] = DAO_Buffer[0] + (DAO_Buffer[1] << 8);
    first4Bytes[1] = DAO_Buffer[2] + (DAO_Buffer[3] << 8);

    IPv6Address source = ((IPv6InterfaceTable *) getModule(INTERFACE_TABLE_MODULE_NAME))->getInterface(0).getLocalAddress();
    IPv6Address target = DODAGInstance.neighborhood.getPrefParent()->getIpAdress();

    //It should be a difference between storing and non-storing!
    if (storing) {
        LOG_DEBUG("Sending DAO from: "<<source.str().c_str() << " to: " << target.str().c_str());

        if(ICMPv6Layer->sendMessage(target,
                   RPL_ICMP_MESSAGE_TYPE,
                   DAO_TYPE,
                   first4Bytes,
                   &DAO_Buffer[4],
                   length - 4)){
            LOG_INFO("DAO Sending Successful");
        }else{
            LOG_ERROR("DAO Sending failed");
        }
    }
    else {
        //non-storing

        if (DODAGInstance.root) {
            //Send to the source
        }
        else {
            if(ICMPv6Layer->sendMessage(target,
                       RPL_ICMP_MESSAGE_TYPE,
                       DAO_TYPE,
                       first4Bytes,
                       &DAO_Buffer[4],
                       length - 4)){
                //worked
                LOG_INFO("DAO Sending Successful");
            }else{
                LOG_ERROR("DAO Sending failed");
            }
        }
    }
}

/*
void RPLRouting::DAO_TimerTimeout(cometos::Message *msg) {
    //TODO Good Timing?

    //Timer for triggering DAO messages??

    schedule(msg, &RPLRouting::DAO_TimerTimeout,
            DODAGInstance.DIO_info.DAGConfig.defaultLifetime / 2 + intrand(DODAGInstance.DIO_info.DAGConfig.defaultLifetime));

    LOG_DEBUG("Timeout");

    //increase DAO Trigger Sequence Number to get new DAO
    if(DODAGInstance.root){
        LOG_DEBUG("Increase DTSN");
        DODAGInstance.DIO_info.DTSN++;
        emptyDAO();
    }
}
*/

void RPLRouting::DAO_TimerDelay(cometos::Message *msg){
    if(scheduleDAO && connected){
    	LOG_DEBUG("connected && scheduleDAO == true");
        sendDAO();
    } else {
        if (!connected) {
            LOG_DEBUG("connected == false");
            //sendDIS();  // TODO: I don't know if this is standard compliant, but I want to get connected again soon
        }
        if (!scheduleDAO){
            LOG_INFO("scheduleDAO == false");
        }
    }

    scheduleDAO = false;
    schedule(msg, &RPLRouting::DAO_TimerDelay, DEFAULT_DAO_DELAY / 2 + intrand(DEFAULT_DAO_DELAY));
}

void RPLRouting::txResult(const IPv6Address & dst, const LlTxInfo & info){
    LOG_DEBUG("s=" << info.success() << "|numSuccTx=" << (int) info.getNumSuccessfulTransmissions() << "|numRtx=" << (int) info.getNumRetries());
    ENTER_METHOD_SILENT();
    //ANDREAS SOLVES PROBLEM SHEDULE?
    IPv6Address multicast(IPV6_MUTLICAST);
    if(dst != multicast){
        if (storing) {
            const IPv6Route * routeToDest = routingTable->doLongestPrefixMatch(dst);
            //TODO ANDREAS intended?
            if (routeToDest) {
                IPv6Address nexthop = routeToDest->getNextHop();
                if(!info.success()){
                    if(DODAGInstance.neighborhood.findNeighborIndex(nexthop) == NOT_A_NEIGHBOR){
                        //TODO Could be simple death while send!
                        ASSERT(DODAGInstance.neighborhood.findNeighborIndex(nexthop) != NOT_A_NEIGHBOR);
                    }
                    //Check if node "died"
                    if(!DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(nexthop))->reduceExpireTime(DEFAULT_LINK_FAILED_REDUCE)){
                        if(!DODAGInstance.neighborhood.removeNeighbor(DODAGInstance.neighborhood.findNeighborIndex(nexthop))){
                            LOG_DEBUG("Neighbor not deleted, pref parent: " << (int)DODAGInstance.neighborhood.getPrefParentIndex());
                        }
                        if(DODAGInstance.neighborhood.getPrefParentIndex() != NO_PREF_PARENT){
                            IPv6Address neighbor = DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.getPrefParentIndex())->getIpAdress();
                            LOG_DEBUG("Neighbor, pref parent: " <<neighbor.str().c_str());
                        }

                        LOG_DEBUG("Neighbor ip: " <<nexthop.str().c_str() << " died");

                        if(cleanDAO()){
                            scheduleDAO = true;
                        }
                        if(DODAGInstance.neighborhood.getPrefParentIndex() == NO_PREF_PARENT){
                            LOG_DEBUG("PREF_PARENT LOST DUE TO PACKAGE LOSS");
                            choosePrefParent(PREF_PARENT_LOST);
                        }
                    }
                }else{
//                    cometos::getCout()<<"nextHop: " << nexthop.str().c_str()<<cometos::endl;
//                    cometos::getCout()<<"Routing Table next hop " << routingTable->getDefaultRoute()->getNextHop().str().c_str()<<cometos::endl;
                    if(routingTable->getDefaultRoute()->getNextHop() != nexthop){
                        DODAGInstance.neighborhood.refreshNeighbor(nexthop);
                    }
                }
            } else {
                // address not found in routing table
            }
        }
#ifdef COMETOS_V6_RPL_SR
        else {
            const IPv6Route * routeToDest = sourceRoutingTable->doLongestPrefixMatch(dst);
            if (routeToDest) {
                IPv6Address nexthop = routeToDest->getNextHop();
                if(!info.success()){
                    if(DODAGInstance.neighborhood.findNeighborIndex(nexthop) == NOT_A_NEIGHBOR){
                        //TODO Could be simple death while send!
                        ASSERT(DODAGInstance.neighborhood.findNeighborIndex(nexthop) != NOT_A_NEIGHBOR);
                    }
                    //Check if node "died"
                    if(!DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.findNeighborIndex(nexthop))->reduceExpireTime(DEFAULT_LINK_FAILED_REDUCE)){
                        if(!DODAGInstance.neighborhood.removeNeighbor(DODAGInstance.neighborhood.findNeighborIndex(nexthop))){
                            LOG_DEBUG("Neighbor not deleted, pref parent: " << (int)DODAGInstance.neighborhood.getPrefParentIndex());
                        }
                        if(DODAGInstance.neighborhood.getPrefParentIndex() != NO_PREF_PARENT){
                            IPv6Address neighbor = DODAGInstance.neighborhood.getNeighbor(DODAGInstance.neighborhood.getPrefParentIndex())->getIpAdress();
                            LOG_DEBUG("Neighbor, pref parent: " <<neighbor.str().c_str());
                        }

                        LOG_DEBUG("Neighbor ip: " << nexthop.str().c_str() << " died");

                        if(cleanDAO()){
                            scheduleDAO = true;
                        }
                        if(DODAGInstance.neighborhood.getPrefParentIndex() == NO_PREF_PARENT){
                            LOG_DEBUG("PREF_PARENT LOST DUE TO PACKAGE LOSS /n");
                            choosePrefParent(PREF_PARENT_LOST);
                        }
                    }
                }else{
                    LOG_DEBUG("TEST link result! " << nexthop.str().c_str());
                    DODAGInstance.neighborhood.refreshNeighbor(nexthop);
                }
            } else {
                // address not found in routing table
            }
        }
#endif
    }
}

void RPLRouting::rxResult(const IPv6Address & src, const LlRxInfo & info) {
    LOG_DEBUG("rssi=" << (int) info.avgRssi() << "|lqi=" << (int) info.avgLqi() << "|lqiValid=" << info.isLqiValid());
}

bool RPLRouting::cleanDAO(){
    //Update DAO
    bool change = false;
    int i_start;
    if(DODAGInstance.root){
        i_start = 0;
    } else {
        i_start = 1;
    }
    if (storing) {
        for(uint8_t i = i_start; i < DAO_Information.RPL_TargetSize; i++){
            if(routingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[i].target)!= NULL){
                if(DODAGInstance.neighborhood.findNeighborIndex(routingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[i].target)->getNextHop()) == NOT_A_NEIGHBOR){
                    change |= deleteEntry(DAO_Information.RPL_Targets[i].target);
                    if(DAO_Information.deleteTarget(i)){
                        i--;
                        change = true;
                        //LOG_DEBUG("i: " << i);
                    }
                }
            }
        }
    }
#ifdef COMETOS_V6_RPL_SR
    else {
        for(uint8_t i = i_start; i < DAO_Information.RPL_TargetSize; i++){
            if(sourceRoutingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[i].target)!= NULL){
                if(DODAGInstance.neighborhood.findNeighborIndex(sourceRoutingTable->doLongestPrefixMatch(DAO_Information.RPL_Targets[i].target)->getNextHop()) == NOT_A_NEIGHBOR){
                    change |= deleteEntry(DAO_Information.RPL_Targets[i].target);
                    if(DAO_Information.deleteTarget(i)){
                        i--;
                        change = true;
                        //LOG_DEBUG("i: " << i);
                    }
                }
            }
        }
    }
#endif
    return change;
}

void RPLRouting::emptyDAO(){
    //Update DAO
    int i_start;
    if(DODAGInstance.root){
         i_start = 0;
    } else {
         i_start = 1;
    }
    for(uint8_t i = i_start; i < DAO_Information.RPL_TargetSize; i++){
        deleteEntry(DAO_Information.RPL_Targets[i].target);
        if(DAO_Information.deleteTarget(i)){
            i--;
        }
    }
}

//TODO implement
void RPLRouting::setMetric(uint8_t value){

    if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_HOPCOUNT) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = value;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_ETX) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = value;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_LQL) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = value;
    }
}

void RPLRouting::updateMetric(uint8_t value){
    if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_HOPCOUNT) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = value;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_ETX) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = value;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = 0;
    }
    else if (this->DODAGInstance.DIO_info.metricContainer.MCType == RPL_DODAG_MC_LQL) {
        this->DODAGInstance.DIO_info.metricContainer.metric.hops = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.etx = 0;
        this->DODAGInstance.DIO_info.metricContainer.metric.LQL = value;
    }
}

void RPLRouting::printAllRoutes() {
    routingTable->printAllRoutes();
}

//void RPLRouting::finish(){
//    recordScalar("Num DAOs received", stats.numDAO);
//    recordScalar("Num DIOs received", stats.numDIO);
//    recordScalar("Mean DAO's size", stats.sizeDAO);
//    recordScalar("Mean DIO's size", stats.sizeDIO);
//
////    //let's calculate the men value of the DAOs and DIOs size
////    int meanDAOsize=0;
////    int meanDIOsize=0;
////    if (numDAO>300) {
////        //For calculation purposes (300 is the max value of the size array)
////        numDAO=300;
////    }
////    for(int i=0;numDAO;i++){
////        meanDAOsize = meanDAOsize + sizeDAO[i];
////    }
////    if (numDAO==0){
////        meanDAOsize=0;
////    } else {
////        meanDAOsize = meanDAOsize / numDAO;
////    }
////    if (numDIO>300) {
////        //For calculation purposes (300 is the max value of the size array)
////        numDIO=300;
////    }
////    for(int i=0;numDIO;i++){
////        meanDIOsize = meanDIOsize + sizeDIO[i];
////    }
////    if (numDIO==0){
////        meanDIOsize=0;
////    } else {
////        meanDIOsize = meanDIOsize / numDIO;
////    }
//
//}

}

//namespace cometos {
//void serialize(ByteVector & buf, const cometos_v6::RPLRoutingStats & value) {
//    serialize(buf, value.numDAO);
//    serialize(buf, value.sizeDAO);
//    serialize(buf, value.numDIO);
//    serialize(buf, value.sizeDIO);
//}
//void unserialize(ByteVector & buf, cometos_v6::RPLRoutingStats & value) {
//    unserialize(buf, value.sizeDIO);
//    unserialize(buf, value.numDIO);
//    unserialize(buf, value.sizeDAO);
//    unserialize(buf, value.numDAO);
//}
//
//}
