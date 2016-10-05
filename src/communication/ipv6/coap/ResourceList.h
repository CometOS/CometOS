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
 * @author: Marvin Burmeister, Martin Ringwelski
 */

#ifndef RESOURCELIST_H_
#define RESOURCELIST_H_

#include "CoAPResource.h"

namespace cometos_v6 {

template<uint8_t N>
class ResourceList {
public:
    ResourceList() {
        for (uint8_t i = 0; i < N; i++) {
            resources[i] = NULL;
        }
    }

    ~ResourceList() {
        for(uint8_t i = 0; i < N; i++){
            if(resources[i] != NULL){
                resources[i] = NULL;
            }
        }
    }

    bool deleteResource(CoAPResource* deleteMe){
        if(deleteMe == NULL) {
            return false;
        }

        for(uint8_t i = 0; i < N; i++)
        {
            if(resources[i] == deleteMe){
                delete deleteMe;
                resources[i] = NULL;
                return true;
            }
        }
        return false;
    }

    CoAPResource* findMatchingResource(CoAPMessage* request){
        char* path = NULL;
        uint16_t pathLen = request->getPathString(path);

        if (pathLen == 0) return NULL;

        LOG_DEBUG(path);

        for(uint8_t i = 0; i < N; i++) {
            if ((resources[i] != NULL) && (resources[i]->hasPath(path))) {
                delete[] path;
                return resources[i];
            }
        }

        delete[] path;
        return NULL;
    }

    bool insertResource(CoAPResource* insertMe, CoAPLayer* owner, CoAPBuffer_t* buffer){
        if(insertMe == NULL) {
            return false;
        }

        for(uint8_t i = 0; i < N; i++)
        {
            if(resources[i] == NULL){
                insertMe->setBuffer(buffer);
                insertMe->init(owner);
                resources[i] = insertMe;
                return true;
            }
        }
        return false;
    }

    uint16_t getResourceDiscoveryList(char*& payload) {
        uint16_t pathLength = 0;                                //and send their paths as a response

        for(uint8_t i = 0; i < N; i++){            //To do so, we first need the size of all strings added
            if(resources[i] != NULL &&
                    !resources[i]->isHiddenFromResourceDiscovery())
            {
                pathLength += resources[i]->getPathLength() + 3;    //pathLength + strlen(<>,)
                if(resources[i]->getResourceType() != NULL &&
                        strlen(resources[i]->getResourceType()) > 0)
                    pathLength += strlen(resources[i]->getResourceType()) + 6; //pathLength + strlen(;rt="")
            }
        }

        if (pathLength == 0) return 0;

        uint16_t skip = 0;
        payload = new char[pathLength];                         //allocate a buffer with that size
        for(uint8_t i = 0; i < N; i++){            //and parse the paths into a simple CoRE Link Format
            if(resources[i] != NULL &&
                    resources[i]->getResourceType() != NULL &&
                    !resources[i]->isHiddenFromResourceDiscovery())
            {
                payload[skip] = '<';
                resources[i]->printPath(&payload[++skip],
                        resources[i]->getPathLength());
                skip += resources[i]->getPathLength();
                payload[skip] = '>';

                if(strlen(resources[i]->getResourceType())){
                    payload[++skip] = ';';
                    payload[++skip] = 'r';
                    payload[++skip] = 't';
                    payload[++skip] = '=';
                    payload[++skip] = '"';
                    memcpy(&payload[++skip],
                            resources[i]->getResourceType(),
                            strlen(resources[i]->getResourceType()));
                    skip += strlen(resources[i]->getResourceType()) - 1;
                    payload[skip] = '"';
                }

                payload[++skip] = ',';
                skip++;
            }
        }
        payload[skip - 1] = '\0';

        return pathLength;
    }

protected:
    CoAPResource* resources[N];
};

}

#endif /* RESOURCELIST_H_ */
