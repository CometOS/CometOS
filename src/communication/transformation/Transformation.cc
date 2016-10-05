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

#include "Transformation.h"
#include "Airframe.h"
#include "NetworkTime.h"
#include "OutputStream.h"
#include "logging.h"
#include "MacAbstractionBase.h"
#include "palFirmware.h"
#include "palId.h"

#define NET_PROTOCOL_NAME "tcpwy"

/*METHOD DEFINITION----------------------------------------------------------*/


namespace cometos {

Transformation::Transformation() :
        pGateControlIn(this, &Transformation::handleControlMsg, "gateControlIn"),
        pGateControlOut(this, "gateControlOut"),
        mNeighborhoodModule(nullptr),
        mAlgo(nullptr),
        mReportCounter(0),
        mReportThreshold(0),
        mReceivedMsgs(0),
        mActive(0)
{
}

void Transformation::initialize() {
	Layer::initialize();
#ifdef SIM_API
	mNeighborhoodModule = (TCPWY*)(this->getParentModule()->getSubmodule(NET_PROTOCOL_NAME));
#endif
	/*
	 * TODO: make check whether NeighborhoodModule & Algorithm have been set
	 */
}

// FROM BELOW!
void Transformation::handleIndication(DataIndication* msg) {
    delete msg;
}

void Transformation::resp(DataResponse *response) {
    delete response;
}

void Transformation::handleControlMsg(DataIndication* msg) {
    TCPWYControlHeader header;
    msg->getAirframe() >> header;
    if(header.pControlType == TCPWY_CONTROL_REMOVE) {
        delete msg;
    } else {
        delete msg;
    }
}

void Transformation::setNetworkModul(TCPWY* module) {
    mNeighborhoodModule = module;
}

void Transformation::setAlgorithm(DistAlgorithm* algorithm) {
    mAlgo = algorithm;
}

void Transformation::setAlgoAnalysis(DistAlgoAnalysis* analyser) {
    mAlgoAnalysis = analyser;
}

void Transformation::countReceivedMsg(bool fromNeighbor) {
    mReceivedMsgs++;
}

void Transformation::setReportThreshold(uint16_t number) {
    mReportThreshold = number;
    mReportCounter = mReportThreshold;
    // sets counter so that next occurrence triggers report
    if(mReportCounter > 0) {
        mReportCounter--;
    }
}

void Transformation::printInit(uint8_t algoType, uint8_t analyseType) {
    getCout() << (long)NetworkTime::get() << ";TRF_I;v[" << (int)palId_id() << "];trf=TRF;alg="<< (int)algoType << ";anl=" << (int)analyseType << "!\n";
}

void Transformation::activate() {
    mActive = true;
}

bool Transformation::isActive() {
    return mActive;
}

}// namespace cometos
