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

#ifndef TRANSFORMATION_H_
#define TRANSFORMATION_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Layer.h"
#include "TZTypes.h"
#include "TCPWY.h"
#include "DistAlgorithm.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class Transformation : public cometos::Layer {
public:
    Transformation();

	/**Sets parameters*/
    virtual void initialize();

    virtual void neighborDataUpdateTimer(Message *timer) = 0;
    virtual void timedReport(Message *timer) = 0;

    virtual bool performStep() = 0;

	virtual void handleIndication(DataIndication* msg);
    virtual void resp(DataResponse *response);
	virtual void handleControlMsg(DataIndication* msg);

    virtual void setNetworkModul(TCPWY* module);
    virtual void setAlgorithm(DistAlgorithm* algorithm);
    virtual void setAlgoAnalysis(DistAlgoAnalysis* analyser);

    virtual void countReceivedMsg(bool fromNeighbor);
    virtual void setReportThreshold(uint16_t number);

    virtual void printInit(uint8_t algoType, uint8_t analyseType);

    void activate();
    bool isActive();

    InputGate<DataIndication>  pGateControlIn;
    OutputGate<DataRequest>  pGateControlOut;

protected:
    TCPWY * mNeighborhoodModule;

    DistAlgorithm * mAlgo;
    DistAlgoAnalysis * mAlgoAnalysis;

    // counts to now when to print the whole report
    uint16_t mReportCounter;
    uint16_t mReportThreshold;
    uint16_t mReceivedMsgs;
    bool mActive;
};

} // namespace cometos

#endif /* TRANSFORMATION_H_ */
