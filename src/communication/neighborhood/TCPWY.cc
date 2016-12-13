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

//#define OUTPUT_WEIGELT
//#define OUTPUT_KOTTBUS
/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Define_Module(TCPWY);

TCPWY::TCPWY() :
        pGateControlIn(this, &TCPWY::handleControl, "gateControlIn"),
        pGateControlOut(this, "gateControlOut"),
        frame(NULL),
        seqNum(0),
        toggle(true)
{
}

void TCPWY::initialize() {
	Layer::initialize();
	mHasUpperLayer = false;
	mSendTCAHeader = true;
	if(gateIndOut.isConnected()) {
	    mHasUpperLayer = true;
	}
    tca.initialize(palId_id());
    time_t randomBackoff = intrand(TCA_RANDOM_BACKOFF);
	schedule(new Message, &TCPWY::neighborDataUpdateTimer, TCA_TIME_INTERVAL + randomBackoff);
    //schedule(new Message, &TCPWY::timedReport, intrand(TCA_REPORT_INIT_TIME));

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

    // setting next send msg to have TCA Header with it
    mSendTCAHeader = true;

    // only sends TCA packet when there is no upper layer
    // or if it has an upper layer but hasn't send within the last time frame
    if(!mHasUpperLayer || (mHasUpperLayer && !mHasSend)) {
        Airframe *msg = new Airframe();
        //Sending out @ every tick
        TCPWYHeader	header(TCPWY_MSG_TYPE_DATA, seqNum);
        setHeaderData(header);

        (*msg) << header;
        sendRequest(new DataRequest(BROADCAST, msg, createCallback(&TCPWY::resp)));
        mSendTCAHeader = false;
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
    getCout() << (long)NetworkTime::get() << ";TCA_T;v[" << (int)palId_id()
            << "];c={";
    bool atLeastOne = false;
    for(uint8_t i = 0; i < NEIGHBORLISTSIZE+STANDBYLISTSIZE; i++) {
        if(tca.neighborView[i].id != TZ_INVALID_ID) {
            if(!tca.neighborView[i].onNL) {
                if(atLeastOne) {
                    getCout() << ",";
                }
                getCout() << "(" << (int)tca.neighborView[i].id << "," << (int)tca.neighborView[i].qualityLT << ")";
                atLeastOne = true;
            }
        }
    }
    getCout() << "}!\n";

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
    if(checkMsgType(msg->getAirframe())) {
        TCPWYHeader header;
        msg->getAirframe() >> header;
        tca.handle(msg->src, header, NetworkTime::get());
        // only forwards messages if node and sender are on each others neighbor lists
        if(mHasUpperLayer && isFromNeighbor && (msg->getAirframe().getLength() > 0)) {
            sendIndication(msg);
        } else {
            delete msg;
        }
    } else {
        // if there is no TCA header yet there is module in a higher layer it just forwards the msg
        // if it is from a neighbor
        if(mHasUpperLayer && isFromNeighbor) {
            sendIndication(msg);
        } else {
            delete msg;
        }
    }
}

void TCPWY::handleRequest(DataRequest* msg){
    mHasSend = true;
    //Sending out @ every tick
    if(mSendTCAHeader) {
        mSendTCAHeader = false;
        TCPWYHeader header(TCPWY_MSG_TYPE_DATA, seqNum);
        setHeaderData(header);
        msg->getAirframe() << header;
    }
    sendRequest(msg);
}

void TCPWY::sendControl(uint8_t controlType, node_t targetId) {
    //Sending out whenever needed
    Airframe *msg = new Airframe();
    TCPWYControlHeader header(controlType, targetId);
    (*msg) << header;
    pGateControlOut.send(new DataIndication(msg,palId_id(),palId_id()));
}

void TCPWY::handleControl(DataRequest* msg) {
    // for now there are no active control requests higher layers can make
    delete msg;
}

void TCPWY::setHeaderData(TCPWYHeader &header){
    header.seqNum = seqNum;
    header.msgType = TCPWY_MSG_TYPE_DATA;
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
}

bool TCPWY::checkMsgType(Airframe& msg) {
    // checks whether the msg is of type TCPWY
    bool result = false;
    uint8_t msgType = 0;
    msg >> msgType;

    if(msgType == TCPWY_MSG_TYPE_DATA) {
        result = true;
    }
    msg << msgType;
    return result;
}

}// namespace cometos
