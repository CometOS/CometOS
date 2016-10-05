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
 * @author Fabian Krome
 */

#ifndef RPLROUTING_H_
#define RPLROUTING_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "RoutingBase.h"
#include "RPLBasics.h"
#include "types.h"
#include "cometos.h"
#include "RPLObjectiveFunction.h"
#include "TrickleModule.h"
#include "ICMPv6Message.h"
#include "ICMPv6.h"
#include "RPLRoutingTable.h"
#include "RPLSourceRoutingTable.h"
#include "Module.h"
namespace cometos_v6 {

#define RPL_MODULE_NAME "rm"


/*MACROS---------------------------------------------------------------------*/
#ifndef RPL_DEFAULT_DIO_ROOT
    #define RPL_DEFAULT_DIO_ROOT false
#endif

#define RPL_SCALAR(x)           uint16_t x
#define RPL_SCALAR_INI(x, y)    x(y)
#define RPL_SCALAR_INC(x)       stats.x++

struct RPLRoutingStats {
    RPLRoutingStats() :
        RPL_SCALAR_INI(numDAO, 0),
        RPL_SCALAR_INI(sizeDAO, 0),
        RPL_SCALAR_INI(numDIO, 0),
        RPL_SCALAR_INI(sizeDIO, 0)
    {}

    void reset() {
        numDAO = 0;
        sizeDAO = 0;
        numDIO = 0;
        sizeDIO = 0;
    }

    RPL_SCALAR(numDAO);
    RPL_SCALAR(sizeDAO);
    RPL_SCALAR(numDIO);
    RPL_SCALAR(sizeDIO);
};

/*Predefines----------------------------------------*/


/**-------------------------------------**/

const uint8_t  RPL_ICMP_MESSAGE_TYPE =  155;


//TODO
#define IPV6_MUTLICAST                  0xff02, 0,0,0,0,0,0,0x1a
#define NO_NEIGHBOR_METRIC_HOP          10


enum parentChangeCause {NO_CAUSE, PREF_PARENT_LOST, NEW_DODAG_VERSION};



/*-----------------------------------------------------------------*/


/** IPv6 Routing Protocol for Low-Power and Lossy Networks
 *
 */

/*Classes---------------------------------------------------------------------*/



/*************************************************************/
/*
 * Main Class
 * RPLRouting Class with trickleModule and RoutingBase-class
 */

class RPLRouting : public cometos::TrickleModule, public RoutingBase, public ICMPv6Listener {
public:

    RPLRouting(const char * name = NULL);

    ~RPLRouting();

    // initializes instance with
    void initialize();

    void initModules();

    void initObjFunc();

    // finish
//    void finish();

    //Initializes a new DODAG (as root)
    void rootDODAG();

    //MESSAGE HANDLER ********************************
    void icmpMessageReceived(const IPv6Address &src, const IPv6Address & dst, uint8_t type, uint8_t code, const uint16_t additionalValues[2] , const uint8_t *data, uint16_t length);

    void DAO_TimerTimeout(cometos::Message *msg);

    void DAO_TimerDelay(cometos::Message *msg);

    void printAllRoutes();

    virtual void txResult(const IPv6Address & dst, const LlTxInfo & info);

    virtual void rxResult(const IPv6Address & src, const LlRxInfo & info);

    void secondStageInitialize(cometos::Message* msg);

    // Storing / NonStoring
    bool storing;

    // Stats
//    RPLRoutingStats getStats() {
//        return stats;
//    }
//    void resetStats() {
//        stats.reset();
//    }

    //Send DIS
    void sendDIS();

private:
    //ROUTING TABLE*************************************

    //delete Entry
    bool deleteEntry(IPv6Address ip);

    //modify Entry
    bool addOrModifyEntry(IPv6Address *
            destPrefix, uint8_t lengthPrefix, const IPv6Address * nextHop, int metric);


    //SENDING OPERATIONS*********************************
    //Send DIO
    void sendDIO();

    //Send DAO
    void sendDAO();

    //MESSAGE HANDLER ********************************
    //receive DIO
    void handleDIO(const IPv6Address &src, const uint8_t *data, uint16_t length);

    //receive DIS
    void handleDIS(const uint8_t *data, uint16_t length);

    //receive DAO
    void handleDAO(const IPv6Address &src, const uint8_t *data, uint16_t length);

    //HELPERS ****************************************

    //Update Internals
    void DAG_ConfigUpdate();


    //Timeout function trickle
    void transmit();

    //NEIGHBORHOOD HANDLING ***************************
    //Updates if neighbor is a parent - returns if preferred parent is lost
    bool updateParentSet();

    //Checks if a node is a parent-node and sets as parent
    void checkIfParent(RPLNeighbor *neighbor);

    //Choose preferred parent of node
    void choosePrefParent(parentChangeCause);

    //Adds new found node into neighborhood
    uint8_t addNeighbor(uint8_t rank, uint16_t DODAGVersionNumber, DAG_MC metricValue, IPv6Address ip);



    bool cleanDAO();

    void emptyDAO();

    //INTERNALS *********************************************
    bool scheduleDAO;

    DODAG_Object DODAGInstance;
    DAO_Object DAO_Information;

    //IP6Address
    //IPv6Address ip;

    RPLObjectiveFunction* objectiveFunction;

    bool connected;
    bool poisonSubDAG;
    bool neverConnected;

    //TODO delete for define
    ICMPv6 * ICMPv6Layer;
    RPLRoutingTable * routingTable;

#ifdef COMETOS_V6_RPL_SR
    RPLSourceRoutingTable * sourceRoutingTable;
#endif

    //TODO BETTER BUFFER SOLUTION MAGIC NUMBER
    uint8_t DIO_Buffer[MAX_LENGTH];
    uint8_t DAO_Buffer[MAX_LENGTH];

    uint8_t messageReceivedBuffer[MAX_LENGTH];

    uint8_t pathSequence;

    void setMetric(uint8_t value);

    void updateMetric(uint8_t value);

    //Stats
    RPLRoutingStats stats;

    DAO_Object currentDAO_info;

    //MSG
    cometos::Message msg;

public:
    bool isRoot = DODAGInstance.root;
};

/*****************************************************************************************************/

}

namespace cometos {
void serialize(ByteVector & buf, const cometos_v6::RPLRoutingStats & value);
void unserialize(ByteVector & buf, cometos_v6::RPLRoutingStats & value);
}

#endif /* RPLROUTING_H_ */
