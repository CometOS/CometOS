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
 * @author Bastian Weigelt
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "LWT.h"
#include "Airframe.h"
#include "mac_interface.h"
#include "NetworkTime.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"
#include "palId.h"
#ifdef SIM_API
#include "palLocation.h"
#endif
//#include "palLocation.h"
#include "DistAlgoAnalysis.h"

#define NET_PROTOCOL_NAME "tcpwy"

#define LWT_TIME_INTERVAL 500 // default: 500
#define LWT_RANDOM_BACKOFF 1000 // default: 1000
#define LWT_RETRY_INTERVAL 100
#define LWT_CHANGE_INTERVAL 100
#define LWT_REPORT_INTERVAL 1000 // default: 1000
#define LWT_REPORT_INIT_TIME 1000
#define LWT_REPORT_COUNTER 1
#define LWT_START_TIME 400000

/*
 * Setting different algorithms
 */
#define ALGORITHM_MIS
#ifdef ALGORITHM_MIS
#include "MISAlgo.h"
#include "MISAnalysis.h"
#endif

//#define ALGORITHM_SPT
#ifdef ALGORITHM_SPT
#include "SPT2Algo.h"
#include "SPT2Analysis.h"
#define SPT_ROOT_ID 10
#endif

//#define ALGORITHM_COLOR
#ifdef ALGORITHM_COLOR
#include "ColorAlgo.h"
#include "ColorAnalysis.h"
#endif

/*
 * Choosing whether and what node to stop during execution
 */
//#define STOP_NODE
#ifdef STOP_NODE
#define NODE_TO_STOP 17
#define TIME_TO_STOP 140000
#endif

/*
 * Setting output formats
 */
//#define OUTPUT_WEIGELT
//#define OUTPUT_KOTTBUS
/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Define_Module(LWT);

LWT::LWT(){}

void LWT::initialize() {
	Layer::initialize();

    cometos::getCout() << "#ConsoleDump;" << (long)NetworkTime::get() << ";v[" << (int)palId_id() << "]!\n";

	/*
	 * Init Neighborhood-Module and Algorithm
	 */
#ifdef SIM_API
	mNeighborhoodModule = (TCPWY*)(this->getParentModule()->getSubmodule(NET_PROTOCOL_NAME));
#ifdef ALGORITHM_MIS
	MISAlgo* algo = new MISAlgo();
	mAlgo = algo;
#endif
#ifdef ALGORITHM_SPT
    SPT2Algo* algo = new SPT2Algo();
    mAlgo = algo;
#endif
#ifdef ALGORITHM_COLOR
    ColorAlgo* algo = new ColorAlgo();
    mAlgo = algo;
#endif
#endif
    mAlgo->initialize(mNeighborhoodModule->tca.neighborView);
    /*
     * Init Algo-Analyser
     */
    uint8_t algoType = mAlgo->getAlgoType();
    DistAlgoAnalysis* analyser = nullptr;
    mAlgoAnalysis = nullptr;
    // switch regarding type of algorithm
    switch(algoType) {
        case DIST_ALGO_TYPE_MIS:
#ifdef ALGORITHM_MIS
            analyser = new MISAnalysis(mAlgo->getLocalState(), &(mNeighborhoodModule->tca), (MISAlgo*) mAlgo);
#endif
            mAlgoAnalysis = analyser;
            break;
        case DIST_ALGO_TYPE_SPT:
#ifdef ALGORITHM_SPT
#ifdef SIM_API
            algo->setRoot(SPT_ROOT_ID);
#endif
            analyser = new SPT2Analysis(mAlgo->getLocalState(), &(mNeighborhoodModule->tca), (SPT2Algo*) mAlgo);
#endif
            mAlgoAnalysis = analyser;
            break;
        case DIST_ALGO_TYPE_COLOR:
#ifdef ALGORITHM_COLOR
            analyser = new ColorAnalysis(mAlgo->getLocalState(), &(mNeighborhoodModule->tca), (ColorAlgo*) mAlgo);
#endif
            mAlgoAnalysis = analyser;
            break;
        default:
            getCout() << (long)NetworkTime::get() << ";TRF_E;v[" << (int)palId_id() << "];ERROR: Wrong type of Algorithm!\n";
            break;
    }
    //printInit(algoType, analyser->getAnalyserType());
    if(mAlgoAnalysis) {
        mAlgoAnalysis->activate();
    }
    printInit(algoType,mAlgoAnalysis->getAnalyserType());

	/*
	 * TODO: make check whether NeighborhoodModule & Algorithm have been set
	 */

    setReportThreshold(LWT_REPORT_COUNTER);
//    if(LWT_START_TIME > 0) {
//        setReportThreshold(LWT_REPORT_COUNTER * 20);
//    }
	mRandomBackoff = intrand(LWT_RANDOM_BACKOFF);
	mTimerMsg = new Message;
	// start repeating reports
	schedule(mTimerMsg, &LWT::neighborDataUpdateTimer, LWT_TIME_INTERVAL + mRandomBackoff);
	schedule(new Message, &LWT::timedReport, intrand(LWT_REPORT_INIT_TIME));
/* OWN FORMAT */
#ifdef SIM_API
	getCout() << (long)NetworkTime::get() << ";TRF_P;v[" << (int)palId_id() << "];{x=" << PalLocation::getInstance()->getOwnCoordinates().x << ",y=" << PalLocation::getInstance()->getOwnCoordinates().y << "}!\n";
#else
    getCout() << (long)NetworkTime::get() << ";TRF_P;v[" << (int)palId_id() << "];{x=" << (int)palId_id() << ",y=" << (int)70*mRandomBackoff << "}!\n";
#endif
}

void LWT::neighborDataUpdateTimer(Message *timer) {
#ifdef STOP_NODE
    if((palId_id() == NODE_TO_STOP) && (NetworkTime::get() > TIME_TO_STOP)) {
        delete timer;
        return 0;
    }
#endif
    Airframe *msg = new Airframe();
    mRandomBackoff = intrand(LWT_RANDOM_BACKOFF);
    mTimerMsg = new Message;
    schedule(mTimerMsg, &LWT::neighborDataUpdateTimer, LWT_TIME_INTERVAL + mRandomBackoff);
    if(mActive) {
        bool sendMsg = performStep();
        // if node didn't already send a message
        if(!sendMsg) {
            //Sending out @ every tick
            mAlgo->addHeader(*msg);
            sendRequest(new DataRequest(BROADCAST, msg, createCallback(&LWT::resp)));
        }
    } else if(NetworkTime::get() >= LWT_START_TIME) {
        activate();
    }
//    } else if(NetworkTime::get() >= LWT_START_TIME - (3*LWT_REPORT_INTERVAL)){
//        if(NetworkTime::get() >= LWT_START_TIME) {
//            activate();
//        }
//        if(mReportThreshold > LWT_REPORT_COUNTER) {
//            setReportThreshold(LWT_REPORT_COUNTER);
//        }
//    }

    delete timer;
}

void LWT::timedReport(Message *timer) {
    mAlgoAnalysis->countState(mAlgo->getLocalState(), NetworkTime::get());
    if(mReportCounter >= mReportThreshold) {
//if(palId_id() == 35 || palId_id() == 4) {
        mAlgoAnalysis->printAll();
//}
        mReportCounter = 0;
        mReceivedMsgs = 0;
    }
    schedule(new Message, &LWT::timedReport, LWT_REPORT_INTERVAL);
    delete timer;
    mReportCounter++;
}

bool LWT::performStep() {
    bool sendMsg = false;
    // safes current local state
    mAlgo->saveLocalState();
    // evaluates guards
    uint8_t activeGuard = mAlgo->evaluateGuards();
    // if any guard is active
    if(activeGuard != DIST_ALGO_GUARD_NONE) {
        // executes step of algorithm
        bool didExecute = mAlgo->execute(activeGuard);
        if(didExecute) {
            //// broadcasting new state
            sendMsg = true;
            Airframe *msg = new Airframe();
            mAlgo->addHeader(*msg);
            sendRequest(new DataRequest(BROADCAST, msg, createCallback(&LWT::resp)));
        }
    }
    mAlgo->finally();
    return sendMsg;
}

// FROM BELOW!
void LWT::handleIndication(DataIndication* msg) {
    if(mAlgo->checkMsgType(msg->getAirframe())) {
        mAlgo->setCache((*msg));
        delete msg;
        // only handles msgs if algorithm is active
        if(mActive) {
            /*
             * TODO: Random backoff before next step???
             */
            performStep();
        }
    } else {
        delete msg;
    }
}

void LWT::resp(DataResponse *response) {
//    bool hasInfo = false;
//    MacTxInfo* info = new MacTxInfo();
//    try {
//        info = response->get<MacTxInfo>();
//        hasInfo = true;
//    } catch(...) {
//        /* OWN FORMAT */
//        getCout() << (long)NetworkTime::get() << ";TRF_R;node[" << palId_id() << "]" << ";NoInfo!\n";
//    }
//    info->
//    getCout() << "MAC_V;rssi=" << (int)mac_getRssi() << ";tx=" << (int)mac_getTxPower() << "!\n";
    bool wasSuccess = response->isSuccess();
    mAlgoAnalysis->countMessage(wasSuccess);
    mAlgoAnalysis->countState(mAlgo->getLocalState(), NetworkTime::get());
    if(!wasSuccess) {
        // resetting to old state
        mAlgo->revertLocalState();
        cancel(mTimerMsg);
        delete mTimerMsg;
        mRandomBackoff = intrand(2 * LWT_RETRY_INTERVAL);
        mTimerMsg = new Message;
        schedule(mTimerMsg, &LWT::neighborDataUpdateTimer, LWT_RETRY_INTERVAL + mRandomBackoff);
    }

    delete response;
}

void LWT::handleControlMsg(DataIndication* msg) {
    TCPWYControlHeader header;
    bool wasSuccess = false;
    msg->getAirframe() >> header;
    switch(header.pControlType) {
        case TCPWY_CONTROL_REMOVE:
            delete msg;
            if(mActive) {
                performStep();
            }
            break;
        case TCPWY_CONTROL_RECEIVED:
            delete msg;
            countReceivedMsg(false);
            break;
        case TCPWY_CONTROL_RECEIVED_NEIGHBOR:
            delete msg;
            countReceivedMsg(true);
            break;
        case TCPWY_CONTROL_ACTIVATE:
            delete msg;
            activate();
            break;
        case TCPWY_CONTROL_SEND:
            if(header.pTargetId > 0) {
                wasSuccess = true;
            }
            delete msg;
            mAlgoAnalysis->countMessage(wasSuccess);
            break;
        default:
            delete msg;
            break;
    }
}

void LWT::countReceivedMsg(bool fromNeighbor) {
    mReceivedMsgs++;
    mAlgoAnalysis->countReceived();
}

void LWT::printInit(uint8_t algoType, uint8_t analyseType) {
    getCout() << (long)NetworkTime::get() << ";TRF_I;v[" << (int)palId_id() << "];trf=LWT;alg="<< (int)algoType << ";anl=" << (int)analyseType << "!\n";
}

}// namespace cometos
