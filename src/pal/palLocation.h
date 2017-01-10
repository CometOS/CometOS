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
 * @author Florian Kauer
 */

#ifndef PALLOCATION_H_
#define PALLOCATION_H_

#include "types.h"
#include "Airframe.h"

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

    CoordinateType getSquaredDistance(const Coordinates& other) const {
        CoordinateType dist = (x-other.x)*(x-other.x)+(y-other.y)*(y-other.y);
#if LOCATION_DIMENSIONS == 3
        dist += (z-other.z)*(z-other.z);
#endif
        return dist;
    }

    static Coordinates INVALID_COORDINATES;

    bool operator==(const Coordinates& other) const {
        return (other.x == x && other.y == y && other.z == z);
    }

    bool operator!=(const Coordinates& other) const {
        return !operator==(other);
    }
};

inline Coordinates operator-(const Coordinates& c1, const Coordinates& c2) {
    return Coordinates{c1.x - c2.x, c1.y - c2.y, c1.z - c2.z};
}

inline bool areClockwiseIn2D(const Coordinates& v1, const Coordinates& v2) {
    // Check if sign the projection of v2 on the normal of v1
    return -v1.x*v2.y + v1.y*v2.x > 0;
}

inline bool isInsideCircleSectorIn2D(const Coordinates& base, const Coordinates& start, const Coordinates& end, const Coordinates& probe) {
    // Check if the probe vector is counter-clockwise from the start vector and clockwise from the end vector 
    // In contrast to http://stackoverflow.com/questions/13652518/efficiently-find-points-inside-a-circle-sector 
    // this code also handles angles larger than 180 deg
    Coordinates vecStart = start-base;
    Coordinates vecEnd = end-base;
    Coordinates vecProbe = probe-base;

    bool b = !areClockwiseIn2D(vecStart,vecProbe) && areClockwiseIn2D(vecEnd,vecProbe);
    if(areClockwiseIn2D(vecStart,vecEnd)) {
        // The angle from vecStart to vecEnd is larger than 180 deg,
        // equivalent to "it is shorter to go the clockwise way".
        b = !b;
    }
    return b;
}

// see http://stackoverflow.com/questions/14176776/find-out-if-2-lines-intersect
inline bool isIntersectingIn2D(const Coordinates& p1, const Coordinates& p2, const Coordinates& q1, const Coordinates& q2) {
    return (((q1.x-p1.x)*(p2.y-p1.y) - (q1.y-p1.y)*(p2.x-p1.x))
            * ((q2.x-p1.x)*(p2.y-p1.y) - (q2.y-p1.y)*(p2.x-p1.x)) < 0)
            &&
           (((p1.x-q1.x)*(q2.y-q1.y) - (p1.y-q1.y)*(q2.x-q1.x))
            * ((p2.x-q1.x)*(q2.y-q1.y) - (p2.y-q1.y)*(q2.x-q1.x)) < 0);
}

inline cometos::Airframe& operator<<(cometos::Airframe& frame, const Coordinates& value) {
    frame << value.x;
    frame << value.y;
#if LOCATION_DIMENSIONS == 3
    frame << value.z;
#endif
    return frame;
}

inline cometos::Airframe& operator>>(cometos::Airframe& frame, Coordinates& value) {
#if LOCATION_DIMENSIONS == 3
    frame >> value.z;
#endif
    frame >> value.y;
    frame >> value.x;
    return frame;
}

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

    virtual Coordinates getCoordinatesForNode(node_t node) = 0;

    static PalLocation* getInstance();

protected:
    PalLocation() {};
};



}

#endif /* PALLOCATION_H_ */
