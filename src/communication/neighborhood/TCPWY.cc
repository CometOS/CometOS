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
 * @author Gerry Siegemund, Bastian Weigelt
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "TCPWY.h"
#include "Airframe.h"
#include "NetworkTime.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"
#include "palId.h"

#define TCA_TIME_INTERVAL 500 // default: 500
#define TCA_RANDOM_BACKOFF 1000  // default: 1000
#define TCA_REPORT_INIT_TIME 1000
#define TCA_REPORT_TIMER 1000  // default: 1000

#ifdef LOCATION_IN_TCPWYHeader
#include "palLocation.h"
#endif

//#define OUTPUT_WEIGELT
//#define OUTPUT_KOTTBUS
/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Define_Module(TCPWY);

TCPWY::TCPWY() :
        pGateControlIn(this, &TCPWY::handleControl, "gateControlIn"),
        pGateControlOut(this, "gateControlOut"),
        seqNum(0),
        toggle(true)
{
}

void TCPWY::initialize() {
	Layer::initialize();
	mHasUpperLayer = false;
	if(gateIndOut.isConnected()) {
	    mHasUpperLayer = true;
	}
    tca.initialize(palId_id());
    auto randomBackoff = intrand(TCA_RANDOM_BACKOFF);
	schedule(new Message, &TCPWY::neighborDataUpdateTimer, TCA_TIME_INTERVAL + randomBackoff);
    schedule(new Message, &TCPWY::timedReport, intrand(TCA_REPORT_INIT_TIME));

}

//void TCPWY::finish() {
//    /* EXTERNAL FORMAT */
//    getCout() << "Finished TCA on node[" << palId_id() << "]! Neighbors are: {";
//    bool atLeastOne = false;
//    for(int i = 0; i < NEIGHBORLISTSIZE; i++) {
//        if(tca.neighborView[i].onNL) {
//            if(atLeastOne) {
//                getCout() << ",";
//            }
//            getCout() << tca.neighborView[i].id;
//            atLeastOne = true;
//        }
//    }
//    getCout() << "}\n";
//    Layer::finish();
//}

void TCPWY::neighborDataUpdateTimer(Message *timer) {
    // only sends TCA packet when there is no upper layer
    // or if it has an upper layer but hasn't send within the last time frame
    if(!mHasUpperLayer || (mHasUpperLayer && !mHasSend)) {
        AirframePtr msg = make_checked<Airframe>();
        //Sending out @ every tick
        TCPWYHeader	header(seqNum);
        setHeaderData(header);

        (*msg) << header;
        (*msg) << (uint8_t)TCPWY_MSG_TYPE_CTRL;
        sendRequest(new DataRequest(BROADCAST, msg, createCallback(&TCPWY::resp)));
    }

    // updating quality values for all neighbors on list and standby-list
    tca.updateAllQuality();

	//promote or delete every other tick
	if(toggle){
	    bool hasNoNeighbors = true;
	    uint8_t numberRemoved = 0;
	    for (uint8_t i = 0; i < NEIGHBORLISTSIZE + STANDBYLISTSIZE; i++) {
	        node_t removedId = tca.checkDeletion(i, NetworkTime::get());
	        if(removedId != TZ_INVALID_ID) {
	            numberRemoved++;
	            // if node has an upper layer it sends up a message indicating the neighborhood has changed
	            if(mHasUpperLayer) {
	                sendControl(TCPWY_CONTROL_REMOVE, removedId);
	            }
	        }
	        if(hasNoNeighbors) {
	            if(tca.neighborView[i].id != TZ_INVALID_ID) {
                    if(tca.neighborView[i].hasBidirectionalLink()) {
                        hasNoNeighbors = false;
                    }
	            }
	        }
	    }
	    tca.resetClusterId();
	}
	toggle = !toggle;

	// setting indicater to check whether node send within next time frame
    mHasSend = false;

    schedule(new Message, &TCPWY::neighborDataUpdateTimer, TCA_TIME_INTERVAL + intrand(TCA_RANDOM_BACKOFF));
	delete timer;
}

void TCPWY::timedReport(Message *timer) {
    schedule(new Message, &TCPWY::timedReport, TCA_REPORT_TIMER);
    LOG_INFO_PREFIX;
    LOG_INFO_PURE("TCA_T;v[0x" << hex << (int)palId_id() << dec << "];c={");
    bool atLeastOne = false;
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE+STANDBYLISTSIZE; i++) {
        if(tca.neighborView[i].id != TZ_INVALID_ID) {
            if(atLeastOne) {
                LOG_INFO_PURE(",");
            }
            atLeastOne = true;

            char open = '(';
            char close = ')';
            if(tca.neighborView[i].onNL) {
                if(tca.neighborView[i].hasBidirectionalLink()) {
                    open = '<';
                    close = '>';
                }
                else {
                    open = '[';
                    close = ']';
                }
            }
            LOG_INFO_PURE(open << "0x" << hex << (int)tca.neighborView[i].id << ":" << dec << (int)tca.neighborView[i].qualityLT << close);
        }
    }
    LOG_INFO_PURE("}!" << endl);

    delete timer;
}

void TCPWY::resp(DataResponse *response) {
    bool wasSuccess = response->isSuccess();
    sendControl(TCPWY_CONTROL_SEND, wasSuccess);
    //ignore it'll be fine :)
	delete response;
}

//FROM BELOW!
void TCPWY::handleIndication(DataIndication* msg) {
    bool isFromNeighbor = false;
    if(tca.isBidirOnNL(msg->src)) {
        isFromNeighbor = true;
        // sending control note to upper layer to indicate arrival of msg from neighbor
        sendControl(TCPWY_CONTROL_RECEIVED_NEIGHBOR, TZ_INVALID_ID);
    } else {
        // sending control note to upper layer to indicate arrival of msg from not a neighbor
        sendControl(TCPWY_CONTROL_RECEIVED, TZ_INVALID_ID);
    }
    // checks whether msg contains a TCA header
    uint8_t msgType;
    msg->getAirframe() >> msgType;
    if(msgType == TCPWY_MSG_TYPE_CTRL) {
        TCPWYHeader header;
        msg->getAirframe() >> header;
        tca.handle(msg->src, header, NetworkTime::get());
        // only forwards messages if node and sender are on each others neighbor lists
        if(mHasUpperLayer && isFromNeighbor && (msg->getAirframe().getLength() > 0)) {
            sendIndication(msg);
        } else {
            delete msg;
        }
    }
    else if(msgType == TCPWY_MSG_TYPE_DATA) {
        // if there is no TCA header yet there is module in a higher layer it just forwards the msg
        // if it is from a neighbor
        if(mHasUpperLayer && isFromNeighbor) {
            sendIndication(msg);
        } else {
            delete msg;
        }
    }
    else {
        LOG_ERROR("Wrong TCPWY type " << msgType);
    }
}

void TCPWY::handleRequest(DataRequest* msg){
    msg->getAirframe() << (uint8_t)TCPWY_MSG_TYPE_DATA;
    sendRequest(msg);
}

void TCPWY::sendControl(uint8_t controlType, node_t targetId) {
    //Sending out whenever needed
    if(pGateControlOut.isConnected()) {
        AirframePtr msg = make_checked<Airframe>();
        TCPWYControlHeader header(controlType, targetId);
        (*msg) << header;
        pGateControlOut.send(new DataIndication(msg,palId_id(),palId_id()));
    }
}

void TCPWY::handleControl(DataRequest* msg) {
    // for now there are no active control requests higher layers can make
    delete msg;
}

void TCPWY::setHeaderData(TCPWYHeader &header){
    header.seqNum = seqNum;
    header.ccID = tca.pClusterId;
    header.ccDist = tca.pClusterDist;
    uint8_t indexHeader = 0;
    for(uint8_t i=0; i<NEIGHBORLISTSIZE+STANDBYLISTSIZE;i++){
    //no sec-fault check here. There can only be NEIGHBORLISTSIZE .inNL=TRUE
    //-> this is checked elsewhere!
        if (tca.neighborView[i].onNL){
            header.neighbor[indexHeader] = tca.neighborView[i].id;
            indexHeader++;
        } else {
            header.neighbor[indexHeader] = TZ_INVALID_ID;
        }
    }
    seqNum++;

#ifdef LOCATION_IN_TCPWYHeader
    header.coordinates = PalLocation::getInstance()->getOwnCoordinates();
#endif
}

}// namespace cometos
