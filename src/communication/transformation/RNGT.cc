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

#include "RNGT.h"
#include "Airframe.h"
#include "NetworkTime.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"
#include "palId.h"
#ifdef SIM_API
#include "src/core/omnetpp/palLocation.h"
#endif
//#include "palLocation.h"
#include "DistAlgoAnalysis.h"

#define NET_PROTOCOL_NAME "tcpwy"

#define RNGT_HALF_PERIOD 500
#define RNGT_REPORT_INTERVAL 1000
#define RNGT_REPORT_INIT_TIME 200
#define RNGT_REPORT_COUNTER 1
#define MAX_PROBABILITY 10000
#define RNGT_EXECUTE_PROBABILITY 9000
#define RNGT_START_TIME 400000

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

Define_Module(RNGT);

RNGT::RNGT(){}

void RNGT::initialize() {
	Layer::initialize();

    cometos::getCout() << "#ConsoleDump;" << (long)NetworkTime::get() << ";v[" << (int)palId_id() << "]!\n";

    mAlgo = nullptr;
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
    if(mAlgo) {
        mAlgo->initialize(mNeighborhoodModule->tca.neighborView);
    } else {
        getCout() << (long)NetworkTime::get() << ";TRF_E;v[" << (int)palId_id() << "];ERROR: No type of Algorithm!\n";
#ifdef SIM_API
        Layer::halt();
#endif
    }
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
    setReportThreshold(RNGT_REPORT_COUNTER);
//    if(RNGT_START_TIME > 0) {
//        setReportThreshold(RNGT_REPORT_COUNTER * 20);
//    }
    mTimerMsg = new Message;
    // start repeating reports
    schedule(mTimerMsg, &RNGT::neighborDataUpdateTimer, RNGT_HALF_PERIOD);
    schedule(new Message, &RNGT::timedReport, RNGT_REPORT_INIT_TIME);
/* OWN FORMAT */
#ifdef SIM_API
    getCout() << (long)NetworkTime::get() << ";TRF_P;v[" << (int)palId_id() << "];{x=" << palLocation_getCartesianX() << ",y=" << palLocation_getCartesianY() << "}!\n";
#else
    getCout() << (long)NetworkTime::get() << ";TRF_P;v[" << (int)palId_id() << "];{x=" << (int)palId_id() << ",y=" << (int)70*intrand((int)palId_id) << "}!\n";
#endif
}

void RNGT::neighborDataUpdateTimer(Message *timer) {
#ifdef STOP_NODE
    if((palId_id() == NODE_TO_STOP) && (NetworkTime::get() > TIME_TO_STOP)) {
        delete timer;
        return 0;
    }
#endif
    // only acts when algorithm is active
    if(mActive) {
        performStep();
        // send message after random delay [1/4 T, 3/4T]
        uint16_t randomSendDelay = 0.5 * RNGT_HALF_PERIOD + intrand(RNGT_HALF_PERIOD);
        schedule(new Message, &RNGT::sendTimedMsg, randomSendDelay);
    } else if(NetworkTime::get() >= RNGT_START_TIME) {
        activate();
    }
//    } else if(NetworkTime::get() >= RNGT_START_TIME - (3*RNGT_REPORT_INTERVAL)){
//        if(NetworkTime::get() >= RNGT_START_TIME) {
//            activate();
//        }
//        if(mReportThreshold > RNGT_REPORT_COUNTER) {
//            setReportThreshold(RNGT_REPORT_COUNTER);
//        }
//    }
    // schedule next step
    mTimerMsg = new Message;
    schedule(mTimerMsg, &RNGT::neighborDataUpdateTimer, 2* RNGT_HALF_PERIOD);

    delete timer;
}

void RNGT::timedReport(Message *timer) {
    mAlgoAnalysis->countState(mAlgo->getLocalState(), NetworkTime::get());
    if(mReportCounter >= mReportThreshold) {
        mAlgoAnalysis->printAll();
        mReportCounter = 0;
        mReceivedMsgs = 0;
    }
    schedule(new Message, &RNGT::timedReport, RNGT_REPORT_INTERVAL);
    delete timer;
    mReportCounter++;
}

bool RNGT::performStep() {
    bool sendMsg = false;
    // evaluates guards
    uint8_t activeGuard = mAlgo->evaluateGuards();
    // if any guard is active
    if(activeGuard != DIST_ALGO_GUARD_NONE) {
        uint16_t executeProb = intrand(MAX_PROBABILITY);
        if(executeProb < RNGT_EXECUTE_PROBABILITY) {
            getCout() << (long)NetworkTime::get() << ";TRF_N;v[" << (int)palId_id() << "];p=" << (int)executeProb << "!\n";
            // executes step of algorithm
            mAlgo->execute(activeGuard);
        }
    }
    mAlgo->finally();
    return sendMsg;
}

void RNGT::sendTimedMsg(Message *timer) {
    delete timer;
    Airframe *msg = new Airframe();
    mAlgo->addHeader(*msg);
    sendRequest(new DataRequest(BROADCAST, msg, createCallback(&RNGT::resp)));
}

// FROM BELOW!
void RNGT::handleIndication(DataIndication* msg) {
    if(mActive) {
        mAlgo->setCache((*msg));
    }
    delete msg;
}

void RNGT::resp(DataResponse *response) {
    bool wasSuccess = response->isSuccess();
    mAlgoAnalysis->countMessage(wasSuccess);
    mAlgoAnalysis->countState(mAlgo->getLocalState(), NetworkTime::get());
    delete response;
}

void RNGT::handleControlMsg(DataIndication* msg) {
    TCPWYControlHeader header;
    bool wasSuccess = false;
    msg->getAirframe() >> header;
    switch(header.pControlType) {
        case TCPWY_CONTROL_REMOVE:
            delete msg;
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

void RNGT::countReceivedMsg(bool fromNeighbor) {
    mReceivedMsgs++;
    mAlgoAnalysis->countReceived();
}

void RNGT::printInit(uint8_t algoType, uint8_t analyseType) {
    getCout() << (long)NetworkTime::get() << ";TRF_I;v[" << (int)palId_id() << "];trf=RNG;alg="<< (int)algoType << ";anl=" << (int)analyseType << "!\n";
}

}// namespace cometos
