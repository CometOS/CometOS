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
 * CometOS Platform Abstraction Layer for the retrieving the location of a node.
 *
 * @author Stefan Unterschuetz
 * @author Florian Meier (as class)
 */

#ifndef PALLOCATION_H_
#define PALLOCATION_H_

#include "types.h"

namespace cometos {

#ifndef LOCATION_DIMENSIONS
#define LOCATION_DIMENSIONS 3
#endif

#ifndef LOCATION_COORDINATE_BYTES
#define LOCATION_COORDINATE_BYTES 8
#endif

#if LOCATION_COORDINATE_BYTES == 2
typedef int16_t CoordinateType;
#elif LOCATION_COORDINATE_BYTES == 8
typedef int64_t CoordinateType;
#else
#error "Other coordinate byte widths than 2 and 8 are not yet supported"
#endif

class Coordinates {
public:
    CoordinateType x;
    CoordinateType y;
#if LOCATION_DIMENSIONS == 3
    CoordinateType z;
#endif
};

class PalLocation {
public:
    /**
     * Initializes module.
     * Set reference point and scale so all coordinates fit into int16_t.
     * The parameters have to be equal for all nodes in the network!
     *
     * @param refX X reference coordinate in cartesian coordinates (unit millimeter)
     * @param refY Y reference coordinate in cartesian coordinates (unit millimeter)
     * @param refZ Z reference coordinate in cartesian coordinates (unit millimeter)
     * @param scale Scale factor in mm per scaled unit
     */
    virtual void init() = 0;

    /**
     * @return coordinates of node
     */
    virtual Coordinates getOwnCoordinates() = 0;

    static PalLocation* getInstance();

protected:
    PalLocation() {};
};

}

#endif /* PALLOCATION_H_ */
