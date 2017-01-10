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

#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include "HashMap.h"
#include "palId.h"

namespace cometos {

Coordinates Coordinates::INVALID_COORDINATES{std::numeric_limits<CoordinateType>::max(),std::numeric_limits<CoordinateType>::max(),std::numeric_limits<CoordinateType>::max()};

class PalLocationImpl : public PalLocation {
private:
    cometos::HashMap<node_t, Coordinates, LOCATION_NUM_POSITIONS> positions;

public:
    PalLocationImpl() : PalLocation() {
        char str[] = LOCATION_POSITIONS;
        uint16_t maxlen = strlen(str);
        uint16_t pos = 0;

        for(uint16_t i = 0; i < LOCATION_NUM_POSITIONS; i++) {
            long int vals[4];
            for(uint8_t j = 0; j < 4; j++) {
                char* numstr = &str[pos];
                for(; pos < maxlen; pos++) {
                    // find next token
                    if(str[pos] == ',' or str[pos] == ':') {
                        break;
                    } 
                }
                str[pos] = '\0'; // also safe for pos == maxlen
                vals[j] = strtol(numstr,NULL,0);

                // skip the token
                if(pos < maxlen) {
                    pos++;
                }
            }

            node_t id = vals[0];
            Coordinates coord{vals[1],vals[2],vals[3]};
            positions.set(id,coord);
            getCout() << id << " " << vals[1] << " " << vals[2] << " " << vals[3] << endl;
        }
    }

    virtual void init() {
    }

    virtual Coordinates getOwnCoordinates() {
        return getCoordinatesForNode(palId_id());
    }

    virtual Coordinates getCoordinatesForNode(node_t node) {
        Coordinates* c = positions.get(node);
        if(c != nullptr) {
            return *c;
        }
        return Coordinates::INVALID_COORDINATES;
    }
};

PalLocation* PalLocation::getInstance() {
    // Instantiate class
    static PalLocationImpl location;
    return &location;
}

}
