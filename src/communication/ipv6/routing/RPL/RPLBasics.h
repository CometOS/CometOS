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


#ifndef RPLBASICS_H_
#define RPLBASICS_H_

#include "types.h"
#include "cometos.h"

#include "IPv6Datagram.h"
#include "IPv6Address.h"

namespace cometos_v6 {

const uint16_t MAX_LENGTH =         250;

const timeOffset_t ONE_SECOND =               1000;
//DEFINES TODO HIGHER IN HERACHIE?
const uint8_t MAX_RPL_TARGETS =         20;
const uint8_t MAX_DAO_TRANSIENT_INFOS = 5;      //TODO: Adjust to expected neighbors

/*Defines for DODAG-----------------------------------------------------------*/

/*Predefines*/
const uint8_t EXPIRE_TIME =     20;
const uint8_t INTERFACE_ID =    0;

//TODO
const uint8_t DEFAULT_METRIC =  1;

#define IS_ROOT              1
const uint16_t INFINITE_RANK =  0xFFFE;
const uint8_t IP6_BYTES      =  16;
const uint8_t BYTE_SHIFT     =  8;
#define NO_PATH_LIFETIME     0x00000000

const uint8_t DIO_MIN_LENGTH =              24;
const uint8_t DODAG_CONFIG_LENGTH =         14;
const uint8_t DAG_METRIC_CONTAINER_LENGTH=  6;
const uint8_t PREFIX_INFO_LENGTH =          30;
const uint8_t RPL_TARGET_FLAGS_PREFIX =     2;
const uint8_t STORING_TRANSIT_LENGTH =      4;
const uint8_t NON_STORING_TRANSIT_LENGTH =  18;

const uint8_t  MIN_DAO_LENGTH =             4;
enum {
    PAD1 =                  0x00,
    PADN =                  0x01,
    DAG_METRIC_CONTAINER =  0x02,
    ROUTE_INFORMATION =     0x03,
    DODAG_CONFIGURATION =   0x04,
    RPL_TARGET =            0x05,
    TRANSIT_INFORMATION =   0x06,
    SOLICITED_INFORMATON =  0x07,
    PREFIX_INFORMATION =    0x08,
    RPL_TARGET_DESCRIPTOR = 0x09
};

enum {
    DIS_TYPE =              0,
    DIO_TYPE =              1,
    DAO_TYPE =              2,
    DAO_ACK_TYPE =          3
};

const uint8_t DEFAULT_GLOBAL_REPAIR =       30;
const uint8_t DIS_SEND_INTERVAL =           10;
const uint8_t MAX_NR_DODAGS =               5;
const uint16_t DEFAULT_MIN_ADV_RANK = INFINITE_RANK;

const uint8_t RPL_DEFAULT_INSTANCE =        0;
const uint8_t DEFAULT_PATH_CONTROL_SIZE =   0;

//one second too much?
const timeOffset_t DEFAULT_DAO_DELAY =      1000;
//TODO choose reduction by missed package
#define DEFAULT_LINK_FAILED_REDUCE      (DODAGInstance.DIO_info.DAGConfig.defaultLifetime)

const uint8_t DEFAULT_DIO_INTERVAL_MIN =        5;
const uint8_t DEFAULT_DIO_INTERVAL_DOUBLINGS =  20;
const uint8_t DEFAULT_DIO_REDUNDANCY_CONSTANT = 10;
const uint16_t DEFAULT_MIN_HOP_RANK_INCREASE =   256;
const uint16_t DEFAULT_MAX_RANK_INCREASE = 768;

//Mode of Operation
const uint8_t MOP_NO_DOWNWARDROUTES =           0;
const uint8_t MOP_NON_STORING =                 1;
const uint8_t MOP_STORING_NO_MULTICAST =        2;
const uint8_t MOP_STORING_MULTICAST =           3;
const uint8_t DEFAULT_MOP =                     MOP_STORING_MULTICAST;

//ROOT
const uint8_t ROOT_DEFAULT_DTSN =           1;
#define ROOT_DEFAULT_MOP                    DEFAULT_MOP
const uint8_t ROOT_DEFAULT_RPLinstanceID =  1;
const bool ROOT_DEFAULT_grounded =          true;
//TODO METRIC
#define ROOT_DEFAULT_metric                 NULL

/* DODAG Metric Container Object Types */
#define RPL_DODAG_MC_NONE                   0 /* Local identifier for empty MC */
#define RPL_DODAG_MC_NSA                    1 /* Node State and Attributes */
#define RPL_DODAG_MC_ENERGY                 2 /* Node Energy */
#define RPL_DODAG_MC_HOPCOUNT               3 /* Hop Count */
#define RPL_DODAG_MC_THROUGHPUT             4 /* Throughput */
#define RPL_DODAG_MC_LATENCY                5 /* Latency */
#define RPL_DODAG_MC_LQL                    6 /* Link Quality Level */
#define RPL_DODAG_MC_ETX                    7 /* Expected Transmission Count */
#define RPL_DODAG_MC_LC                     8 /* Link Color */

// In MRHOF, rank must differ at least 1/PARENT_SWITCH_THRESHOLD_DIV in order to switch parent
#define PARENT_SWITCH_THRESHOLD_DIV         2

// The ETX in metric_object_t.etx is expressed with a fixed point 16-bit integer
// The `real` value of the ETX can be calculated by dividing with this number
#define RPL_DODAG_MC_ETX_DIVISOR            256

/* OCP for Objective Functions */
#define  RPL_OBJFUNC_OF0                    0
#define  RPL_OBJFUNC_MRHOF                  1

const uint8_t ROOT_DEFAULT_preference =     0x07;
const uint8_t ROOT_DEFAULT_rank =           1;
const uint8_t ROOT_DEFAULT_versionNumber =  0;

/*------------------------------------------------------------------*/
//TODO Metric
// seems to be sensible to provide an abstract base class for all metrics
// and let a factory method or object return a linked list (or maybe a SList)
// of those after parsing the (or multiple) DIO DAG metric container(s);
// this list could be stored in the neighbor table and used by the objective
// function when determining the preferred parent or when sending out DIOs,
// which will have to contain a possibly updated version of the preferred
// parents metric objects
struct metric_object{
    //Hop Count
    uint8_t hops;
    //The Link Quality Level Reliability Metric
    // LQL value from 0 to 7 where 0 means undetermined and 1 indicates
    // the highest link quality
    uint8_t LQL;
    //Expected Transmission Count
    uint16_t etx;

    uint16_t calculateMetricValue (uint8_t MetricType){
        if (MetricType == RPL_DODAG_MC_HOPCOUNT) {
            return hops;
        }
        else if (MetricType == RPL_DODAG_MC_ETX) {
            return etx;
        }
        else if (MetricType == RPL_DODAG_MC_LQL) {
            return LQL;
        }
        else {
            //Hopcount is the metric by default
            return hops;
        }
    }
//    void setMetric(){
//    }

};
typedef struct metric_object metric_object_t;

struct DAG_MC {
    uint8_t MCType;
    uint8_t ResFlags = 0;
    bool P;
    bool C;
    bool O;
    bool R;
    /*A=0: The routing metric is additive
      A=1: The routing metric reports a maximum
      A=2: The routing metric reports a minimum
      A=3: The routing metric is multiplicative
      No meaning if C is set, only valid when R is cleared.
      Otherwise, A must be set to 0 and must be ignored.*/
    uint8_t AField;
    uint8_t PrecField;
    uint8_t length;
    metric_object metric;
};

/*
 * Peer-Node
 */
class RPLNeighbor {
public:

    RPLNeighbor();
    RPLNeighbor(uint16_t rank, uint16_t DODAGVersionNumber, DAG_MC metricValue, IPv6Address ip);

    bool getIsParent();
    uint16_t getRank() const;
    uint16_t getVersion();
    DAG_MC getMetric() const;
    IPv6Address &getIpAdress();
    uint16_t getExpireTime();

    void setIsParent(bool isParent);
    void setRank(uint16_t rank);
    void setVersion(uint16_t DODAGVersionNumber);
    void setMetric( DAG_MC metricValue);
    void setIpAdress(IPv6Address ip);


    //Refreshes Neighbor expire time
    void refreshNeighbor(uint32_t defaultLifetime);

    //Reduce expire Time (true if alive, false if dead)
    bool reduceExpireTime(uint16_t amount);

    bool reduceExpireTimeGobal();

    void setDAOSequence(uint8_t DAOSequence){
        this->DAOSequence = DAOSequence;
    }

    uint8_t getDAOSequence(){
            return DAOSequence;
        }

private:
    bool isParent;
    uint16_t rank;
    uint16_t DODAGVersionNumber;
    DAG_MC metric;  //
    IPv6Address ip;
    uint32_t expireTime;
    uint8_t DAOSequence;
};

/*******************/

/* TODO  max size define*/
const uint8_t MAX_NEIGHBOORHOOD_SIZE =          10;     //testwise decreased to 10, original value 100

const uint8_t NO_PREF_PARENT =                  255;
const uint8_t FULL_NEIGHBOORHOOD =              255;
const uint8_t SAME_IPADDRESS_IN_NEIGHBOORHOOD = 254;
const uint8_t NOT_A_NEIGHBOR =                  255;

//Neighborhood of node
class RPLNeighborhood {
public:
    RPLNeighborhood();
    ~RPLNeighborhood();
    void init();
    RPLNeighbor * getNeighbor(uint8_t index);
    uint8_t getSize();
    uint8_t getPrefParentIndex();
    RPLNeighbor * getPrefParent();

    // NOTE false if selected neighbor not a parent
    bool setPrefParent(RPLNeighbor *parent);
    bool setPrefParent(uint8_t index);

    uint8_t addNeighbor(uint16_t rank, uint16_t DODAGVersionNumber, DAG_MC metricValue, IPv6Address ip);
    bool removeNeighbor(uint8_t index);

    //true if Neigbors removed
    bool removeAllNeighbors();

    uint8_t findNeighborIndex(IPv6Address ip);
    uint8_t findNeighborIndex(RPLNeighbor *neighbor);

    //Reduce expire Time (false if pref parent lost)
    bool reduceGlobalExpireTime();

    void setDefaultLifetime(uint8_t defaultLifetime);

    void setTimeMultiplier(uint16_t timeMultiplier);

    //Refreshes Neighbor expire time
    void refreshNeighbor(const IPv6Address& ip);


private:
    RPLNeighbor neighborhood[MAX_NEIGHBOORHOOD_SIZE];
    uint8_t size;
    uint8_t prefParentIndex;
    uint8_t defaultLifetime;
    uint16_t timeMultiplier;
};

/*************************************************************/



/*Structs/Unions--------------------------------------------------------------*/

//DAG_Configurations from DIO
struct DAG_Config{

    DAG_Config(uint8_t intervalDoublings = DEFAULT_DIO_INTERVAL_DOUBLINGS,
            uint8_t intervalMin = DEFAULT_DIO_INTERVAL_MIN,
            uint8_t redundancyConst = DEFAULT_DIO_REDUNDANCY_CONSTANT,
            bool authentificationEnable = false,
            uint8_t PCS = DEFAULT_PATH_CONTROL_SIZE,
            uint16_t maxRankIncrease = 0,
            uint16_t minHopRankIncrease = DEFAULT_MIN_HOP_RANK_INCREASE,
            uint16_t OCP = 0,
            uint8_t defaultLifetime = 5,
            uint16_t lifetimeUnit = 1):
        intervalDoublings(intervalDoublings),
        intervalMin(intervalMin),
        redundancyConst(redundancyConst),
        authentificationEnable(authentificationEnable),
        PCS(PCS),
        maxRankIncrease(maxRankIncrease),
        minHopRankIncrease(minHopRankIncrease),
        OCP(OCP),
        defaultLifetime(defaultLifetime),
        lifetimeUnit(lifetimeUnit)
    {}

    DAG_Config(const DAG_Config& other):
        intervalDoublings(other.intervalDoublings),
        intervalMin(other.intervalMin),
        redundancyConst(other.redundancyConst),
        authentificationEnable(other.authentificationEnable),
        PCS(other.PCS),
        maxRankIncrease(other.maxRankIncrease),
        minHopRankIncrease(other.minHopRankIncrease),
        OCP(other.OCP),
        defaultLifetime(other.defaultLifetime),
        lifetimeUnit(other.lifetimeUnit)
    {}

    //Trickle timer information
    uint8_t intervalDoublings;
    uint8_t intervalMin;
    uint8_t redundancyConst;

    //Security fields
    bool authentificationEnable;
    uint8_t PCS;

    //Objective function input
    uint16_t maxRankIncrease;
    uint16_t minHopRankIncrease;
    //Type of OF
    uint16_t OCP;

    //Route Lifetime
    uint8_t defaultLifetime;
    uint16_t lifetimeUnit;
};

//information in DIO for decoding
struct DIO_Object{

    DIO_Object(uint8_t RPLinstanceID = ROOT_DEFAULT_RPLinstanceID,
            uint8_t versionNumber = ROOT_DEFAULT_versionNumber,
            uint16_t rank = ROOT_DEFAULT_rank,
            bool grounded = ROOT_DEFAULT_grounded,
            uint8_t MOP = DEFAULT_MOP,
            uint8_t preference = ROOT_DEFAULT_preference,
            uint8_t DTSN = ROOT_DEFAULT_DTSN):
        RPLinstanceID(RPLinstanceID),
        versionNumber(versionNumber),
        rank(rank),
        grounded(grounded),
        MOP(MOP),
        preference(preference),
        DTSN(DTSN)
    {}

    uint8_t RPLinstanceID;
    uint8_t versionNumber;
    uint16_t rank;
    bool grounded;
    uint8_t MOP;
    uint8_t preference;
    // DTSN (Destination Advertisement Trigger Sequence Number)
    uint8_t DTSN;
    IPv6Address DODAGID;

    DAG_MC metricContainer;
    DAG_Config DAGConfig;

};

//DODAG information object
struct DODAG_Object{
    DODAG_Object(bool root = false,
            uint16_t MinAdvertisedRank = DEFAULT_MIN_ADV_RANK,
            uint16_t DIS_SendInterval = DIS_SEND_INTERVAL,
            uint16_t globalRepairInterval = DEFAULT_GLOBAL_REPAIR):
        root(root),
        MinAdvertisedRank(MinAdvertisedRank),
        DIS_SendInterval(DIS_SendInterval),
        globalRepairInterval(globalRepairInterval)
    {}

    DIO_Object DIO_info;

    bool root;

    RPLNeighborhood neighborhood;
    uint16_t MinAdvertisedRank;
    uint16_t DIS_SendInterval;
    uint16_t globalRepairInterval;

};



struct RPL_target{

    RPL_target(): prefixLength(128), transientInfoIndex(0)
    {}

    RPL_target(uint8_t prefixLength, IPv6Address target,
            uint8_t transientInfoIndex):
                prefixLength(prefixLength),
                target(target),
                transientInfoIndex(transientInfoIndex)
    {}

    uint8_t prefixLength;
    IPv6Address target;
    uint8_t transientInfoIndex;

};

struct DAO_TransientInfo{

    DAO_TransientInfo(): E(false), pathControl(0), pathSequence(0),
            pathLifetime(0)
    {}

    DAO_TransientInfo(bool E, uint8_t pathControl,
            uint8_t pathSequence, uint8_t pathLifetime,
            IPv6Address parentAddress):
                E(E),
                pathControl(pathControl),
                pathSequence(pathSequence),
                pathLifetime(pathLifetime),
                parentAddress(parentAddress)
    {}


    //Information about external router //TODO not implemented
    bool E;
    //path Control field
    uint8_t pathControl;
    uint8_t pathSequence;
    uint8_t pathLifetime;

    //TODO non-storing
    IPv6Address parentAddress;
};


struct Temp_Pref_Parent_Object {
    Temp_Pref_Parent_Object(RPL_target target, DAO_TransientInfo info):
    target(target), info(info)
    {}

    RPL_target target;
    DAO_TransientInfo info;
};

//TODO redundancy with routing table!
struct DAO_Object {

    DAO_Object():
        RPLInstanceID(0),
        k(false),
        D(false),
        DAOSequence(0),
        RPL_TargetSize(0),
        DAO_TransientInfoSize(0)
    {}

    uint8_t RPLInstanceID;
    //send ack
    bool k;
    //presents of DODAGID-Field
    bool D;
    uint8_t DAOSequence;
    //ip of root
    IPv6Address DODAGID;

    uint8_t RPL_TargetSize;
    RPL_target RPL_Targets[MAX_RPL_TARGETS];

    uint8_t DAO_TransientInfoSize;
    DAO_TransientInfo DAO_TransientInfos[MAX_DAO_TRANSIENT_INFOS];

    //uint8_t DAO_RPLDescriptor;

    uint8_t findTarget(IPv6Address ip){
        for(uint8_t i = 0; i < RPL_TargetSize; i++){
            if(RPL_Targets[i].target == ip){
                return i;
            }
        }
        return MAX_RPL_TARGETS;
    }

    bool deleteTarget(uint8_t index){

        if(index >= RPL_TargetSize){
            ASSERT(index < RPL_TargetSize);
            return false;
        }

        uint8_t TransInfoIndex = RPL_Targets[index].transientInfoIndex;

        //mark DAO entry as lost -> if already marked delete
        if(DAO_TransientInfos[TransInfoIndex].pathLifetime == NO_PATH_LIFETIME){
            if(index != RPL_TargetSize - 1){
                RPL_Targets[index] = RPL_Targets[RPL_TargetSize - 1];
            }
            RPL_TargetSize--;
            //deleteTransientInfo if not connected to any Target Anymore
            if(DAO_TransientInfoSize == 0){
                ASSERT(DAO_TransientInfoSize != 0);
                return false;
            }

            bool needed = false;
            for(uint8_t i = 0; i < RPL_TargetSize; i++){
                if(RPL_Targets[i].transientInfoIndex == TransInfoIndex){
                    needed = true;
                }
            }
            if(!needed){
                if(TransInfoIndex != DAO_TransientInfoSize - 1){
                    DAO_TransientInfos[TransInfoIndex] = DAO_TransientInfos[DAO_TransientInfoSize - 1];
                }
                DAO_TransientInfoSize--;
            }
            return true;
        } else {

            //mark as lost bust only for this node!
            int needed = 0;
            for(uint8_t i = 0; i < RPL_TargetSize; i++){
                if(RPL_Targets[i].transientInfoIndex == TransInfoIndex){
                    needed++;
                }
            }

            if(needed == 1){
                //transient information only used here
                DAO_TransientInfos[TransInfoIndex].pathLifetime = NO_PATH_LIFETIME;
            } else {
                //more uses
                DAO_TransientInfos[DAO_TransientInfoSize] = DAO_TransientInfos[TransInfoIndex];
                RPL_Targets[index].transientInfoIndex = DAO_TransientInfoSize;
                DAO_TransientInfos[DAO_TransientInfoSize].pathLifetime = NO_PATH_LIFETIME;
                DAO_TransientInfoSize++;

            }

            return true;
        }

    }

    bool compare(DAO_TransientInfo a, DAO_TransientInfo b){
        return  (a.E == b.E) &&
                (a.pathControl == b.pathControl) &&
                (a.pathSequence == b.pathSequence) &&
                (a.pathLifetime == b.pathLifetime);
    }

    bool addOrModifyTarget(DAO_Object *receivedDAO_info, uint8_t ownIndex,
            uint8_t receivedIndex){

        if(ownIndex == MAX_RPL_TARGETS){
            //Add
            if(RPL_TargetSize == MAX_RPL_TARGETS){
                ASSERT(RPL_TargetSize != MAX_RPL_TARGETS);
#ifdef OMNETPP
                std::cout << "RPL_TargetSize == MAX_RPL_TARGETS" << std::endl;
#endif
                return false;
            }
            RPL_Targets[RPL_TargetSize] = receivedDAO_info->RPL_Targets[receivedIndex];
            RPL_TargetSize++;

            //Update Transient Information
            if(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS){
                ASSERT(DAO_TransientInfoSize != MAX_DAO_TRANSIENT_INFOS);
#ifdef OMNETPP
                std::cout << "DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS" << std::endl;
#endif
                return false;
            }
            //Add new Information
            DAO_TransientInfos[DAO_TransientInfoSize] = receivedDAO_info->DAO_TransientInfos[receivedDAO_info->RPL_Targets[receivedIndex].transientInfoIndex];
            //Change transientInfoIndex to own index of Transient information
            RPL_Targets[RPL_TargetSize - 1].transientInfoIndex = DAO_TransientInfoSize;
            DAO_TransientInfoSize++;

        }else {
            //Modify - TODO so far same ip -> transient info change?
            DAO_TransientInfo oldTransInfo = DAO_TransientInfos[RPL_Targets[ownIndex].transientInfoIndex];

            if(compare(receivedDAO_info->DAO_TransientInfos[receivedDAO_info->RPL_Targets[receivedIndex].transientInfoIndex], oldTransInfo)){
                //cout << "compare(receivedDAO_info->DAO_TransientInfos[receivedDAO_info->RPL_Targets[receivedIndex].transientInfoIndex], oldTransInfo)" << endl;
                return false;
            }


            uint8_t TransInfoIndex = RPL_Targets[ownIndex].transientInfoIndex;
            RPL_Targets[ownIndex].transientInfoIndex = MAX_DAO_TRANSIENT_INFOS;

            //deleteTransientInfo if not connected to any Target Anymore
            bool needed;
            for(uint8_t i = 0; i < RPL_TargetSize; i++){
                if(RPL_Targets[i].transientInfoIndex == TransInfoIndex){
                    needed = true;
                }
            }
            if(!needed){
                if(TransInfoIndex != DAO_TransientInfoSize - 1){
                    DAO_TransientInfos[TransInfoIndex] = DAO_TransientInfos[DAO_TransientInfoSize - 1];
                }
                DAO_TransientInfoSize--;
            }

            //Update Transient Information
            if(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS){
                ASSERT(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS);
#ifdef OMNETPP
                std::cout << "DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS" << std::endl;
#endif
                return false;
            }
            //Add new Information
            DAO_TransientInfos[DAO_TransientInfoSize] = receivedDAO_info->DAO_TransientInfos[receivedDAO_info->RPL_Targets[receivedIndex].transientInfoIndex];
            //Change transientInfoIndex to own index of Transient information
            RPL_Targets[ownIndex].transientInfoIndex = DAO_TransientInfoSize;
            DAO_TransientInfoSize++;

        }
        return true;
    }

    bool addOrModifyTarget(Temp_Pref_Parent_Object *receivedDAO_info, uint8_t ownIndex){

        if(ownIndex == MAX_RPL_TARGETS){
            //Add
            if(RPL_TargetSize == MAX_RPL_TARGETS){
                LOG_DEBUG("RPL_TargetSize == MAX_RPL_TARGETS");
                ASSERT(RPL_TargetSize != MAX_RPL_TARGETS);
                return false;
            }
            RPL_Targets[RPL_TargetSize] = receivedDAO_info->target;
            RPL_TargetSize++;

            //Update Transient Information
            if(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS){
                LOG_DEBUG("DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS");
                ASSERT(DAO_TransientInfoSize != MAX_DAO_TRANSIENT_INFOS);
                return false;
            }
            //Add new Information
            DAO_TransientInfos[DAO_TransientInfoSize] = receivedDAO_info->info;
            //Change transientInfoIndex to own index of Transient information
            RPL_Targets[RPL_TargetSize - 1].transientInfoIndex = DAO_TransientInfoSize;
            DAO_TransientInfoSize++;

        }else {
            //Modify - TODO so far same ip -> transient info change?
            DAO_TransientInfo oldTransInfo = DAO_TransientInfos[RPL_Targets[ownIndex].transientInfoIndex];

            if(compare(receivedDAO_info->info, oldTransInfo)){
                //cout << "compare(receivedDAO_info->DAO_TransientInfos[receivedDAO_info->RPL_Targets[0].transientInfoIndex], oldTransInfo)" << endl;
                return false;
            }


            uint8_t TransInfoIndex = RPL_Targets[ownIndex].transientInfoIndex;
            RPL_Targets[ownIndex].transientInfoIndex = MAX_DAO_TRANSIENT_INFOS;

            //deleteTransientInfo if not connected to any Target Anymore
            bool needed;
            for(uint8_t i = 0; i < RPL_TargetSize; i++){
                if(RPL_Targets[i].transientInfoIndex == TransInfoIndex){
                    needed = true;
                }
            }
            if(!needed){
                if(TransInfoIndex != DAO_TransientInfoSize - 1){
                    DAO_TransientInfos[TransInfoIndex] = DAO_TransientInfos[DAO_TransientInfoSize - 1];
                }
                DAO_TransientInfoSize--;
            }

            //Update Transient Information
            if(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS){
                LOG_DEBUG("DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS");
                ASSERT(DAO_TransientInfoSize == MAX_DAO_TRANSIENT_INFOS);
                return false;
            }
            //Add new Information
            DAO_TransientInfos[DAO_TransientInfoSize] = receivedDAO_info->info;
            //Change transientInfoIndex to own index of Transient information
            RPL_Targets[ownIndex].transientInfoIndex = DAO_TransientInfoSize;
            DAO_TransientInfoSize++;

        }
        return true;
    }
};

class RPLCodec {
public:
    static void decodeDAO(DAO_Object &currentDAO_info,  const uint8_t *data, uint16_t length);
    static uint16_t encodeDAO(DIO_Object &currentDIO_info, DAO_Object &currentDAO_info, uint8_t *data, bool K, bool D, bool pad1, uint8_t padN);
    //Decoding
    static void decodeDIO(DIO_Object &currentDIO_info, const uint8_t *data, uint16_t length);
    static uint16_t encodeDIO(uint8_t * data, DIO_Object &currentDIO_info, bool pad1, uint8_t padN, bool DAG_MetricContainer, bool routingInfo, bool DODAG_Config, bool prefixInfo);

    static uint16_t getTrickleImin(uint8_t DIOIntervalMin) {

        // Trickle Imin != DIOIntervalMin, the former is a absolute time unit
        // the latter an exponent: imin = 2^DIOIntervalMin
        ASSERT(DIOIntervalMin < 16);
        uint16_t imin = 1;
        for (uint8_t i=0; i < DIOIntervalMin; i++) {
            imin *= 2;
        }
        return imin;
    }
    /******/
private:
    RPLCodec() {};
    RPLCodec(const RPLCodec & other) {};
    ~RPLCodec() {};
};

}


#endif /* RPLRBASICS_H_ */
