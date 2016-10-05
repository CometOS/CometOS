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
 * Coloring Algorithm
 * @author Bastian Weigelt
 */

#ifndef COLORALGO_H_
#define COLORALGO_H_

#include "ColorData.h"
#include "TZTCAElement.h"
#include "ColorHeader.h"
#include "DistAlgorithm.h"

#define COLOR_MAX NEIGHBORLISTSIZE

#define COLOR_GUARD_CHANGE 1

namespace cometos {

class ColorAlgo : public DistAlgorithm {
public:
    ColorAlgo()
        {

        };

    virtual void initialize(TZTCAElement *neighborhoodList);
    virtual uint8_t evaluateGuards();
    virtual bool execute(uint8_t activeGuard);
    virtual void finally();
    virtual void setCache(DataIndication& msg);
    virtual void saveLocalState();
    virtual void revertLocalState();

    virtual void outReportString();
    virtual void outAlgoAbbrev();

    virtual void addHeader(Airframe& frame);
    virtual bool checkMsgType(Airframe& frame);
    virtual uint8_t getAlgoType();

    uint8_t getMinColor();
    void resetColorAvailable();

    virtual ColorData* getDataPtr();

protected:
    virtual uint8_t getIndexOf(node_t inId);

    ColorData mNeighborStates[NEIGHBORLISTSIZE + STANDBYLISTSIZE];
    bool mColorAvailable[NEIGHBORLISTSIZE];

private:
    uint8_t mBackupColor;
};
}

#endif /* COLORALGO_H_ */
