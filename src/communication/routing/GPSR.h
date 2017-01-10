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
 * @author Florian Kauer
 */

#ifndef GPSR_H_
#define GPSR_H_

#include "Layer.h"
#include "TCPWY.h"
#include "palLocation.h"

namespace cometos {

class GPSRHeader {
public:
    node_t nlSrc;
    node_t nlDst;
    node_t faceStart;
    node_t faceFirstHop;

    bool isGreedy() {
        return faceStart == MAC_BROADCAST;
    }
};

class GPSR: public Layer {
public:

    GPSR();

    void initialize();

    void finish();

    /**@inherit Layer*/
    virtual void handleRequest(DataRequest* msg);

    /**@inherit Layer*/
    virtual void handleIndication(DataIndication* msg);

    void setNeighborhood(TCPWY* neighborhood);

private:
    TCPWY* neighborhood = nullptr;

    void forwardRequest(DataRequest* msg, GPSRHeader& hdr, node_t prevHop);
    node_t getNextGreedyHop(const Coordinates& destinationCoordinates, node_t dst);
    node_t getNextFaceHop(const Coordinates& ownCoordinates, const Coordinates& destinationCoordinates, node_t& faceStart, node_t& faceFirstHop, node_t prevHop);
    bool isPlanarNeighbor(const Coordinates& uCoord, const Coordinates& vCoord, node_t v);
    node_t getNextPlanarNeighborCounterClockwise(const Coordinates& ownCoordinates, const Coordinates& referenceTarget, Coordinates& nextHopCoordinates);
};

} /* namespace cometos */

#endif /* GPSR_H_ */
