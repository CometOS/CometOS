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

#include "palLocation.h"
#include "BaseModule.h"
#include "BaseMobility.h"

namespace cometos {

Coordinates Coordinates::INVALID_COORDINATES{std::numeric_limits<CoordinateType>::max(),std::numeric_limits<CoordinateType>::max(),std::numeric_limits<CoordinateType>::max()};

class PalLocationImpl : public PalLocation {
    virtual void init() {
    }

    Coordinates getCoordinateForModule(omnetpp::cModule* context) {
        BaseMobility* module = nullptr;

        while(context) {
            module = FindModule<BaseMobility*>::findSubModule(context);
            if(module) {
                break;
            }
            context = context->getParentModule();
        }

        ASSERT(module);

        int16_t scale = 1000; // 1 coordinate unit = 1 mm
#if LOCATION_COORDINATE_BYTES == 2
        scale = 100; // 1 coordinate unit = 1 cm
#endif
        Coord coord = module->getMove().getPositionAt(omnetpp::simTime());
        CoordinateType x = ((double)coord.getX())*scale;
        CoordinateType y = ((double)coord.getY())*scale;
#if LOCATION_DIMENSIONS == 3
        CoordinateType z = ((double)coord.getZ())*scale;
        return {x,y,z};
#else
        return {x,y};
#endif
    }

    virtual Coordinates getOwnCoordinates() {
        omnetpp::cModule* context = (omnetpp::cSimulation::getActiveSimulation())->getContextModule();
        return getCoordinateForModule(context);
    }

    virtual Coordinates getCoordinatesForNode(node_t node) {
        omnetpp::cModule* root = (omnetpp::cSimulation::getActiveSimulation())->getContextModule();

        while(root->getParentModule()) {
            root = root->getParentModule();
        }

        omnetpp::cModule* host = nullptr;
        for(omnetpp::cModule::SubmoduleIterator it(root); !it.end(); ++it) {
            if((*it)->hasPar("id") && ((node_t)(*it)->par("id")) == node) {
               host = *it;
               break;
            }
        }

        return getCoordinateForModule(host);
    }
};

PalLocation* PalLocation::getInstance() {
    // Instantiate class
    static PalLocationImpl location;
    return &location;
}

}
