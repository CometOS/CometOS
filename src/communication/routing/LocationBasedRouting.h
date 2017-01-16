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

#ifndef LocationBasedRouting_H_
#define LocationBasedRouting_H_

#include "Layer.h"
#include "TCPWY.h"
#include "palLocation.h"

namespace cometos {

class LocationBasedRoutingHeader {
public:
    node_t nlSrc;
    node_t nlDst;
    node_t faceStart;

    bool isGreedy() {
        return faceStart == MAC_BROADCAST;
    }
};

class LocationBasedRouting: public Layer {
public:

    LocationBasedRouting();

    void initialize();

    void finish();

    /**@inherit Layer*/
    virtual void handleRequest(DataRequest* msg);

    /**@inherit Layer*/
    virtual void handleIndication(DataIndication* msg);

    void setNeighborhood(TCPWY* neighborhood);

private:
    TCPWY* neighborhood = nullptr;

    void forwardRequest(DataRequest* msg, LocationBasedRoutingHeader& hdr);
    node_t getNextGreedyHop(Coordinates& destinationCoordinates, node_t dst);
    node_t getNextFaceHop(Coordinates& ownCoordinates, Coordinates& destinationCoordinates, node_t dst);
    bool isPlanarNeighbor(Coordinates& uCoord, Coordinates& vCoord, node_t v);
};

} /* namespace cometos */

#endif /* LocationBasedRouting_H_ */
