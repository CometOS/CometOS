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
 * @author Andreas Weigel
 */


#ifndef REMOTEACCESSCLIENT_H_
#define REMOTEACCESSCLIENT_H_

#include "RemoteAccess.h"
#include "Airframe.h"
#include "AirString.h"
#include "Callback.h"
#include <tuple>
#include <map>
#include "logging.h"


namespace cometos {

template<typename... Args>
class RemoteAccessSerializer {
public:
    template<int idx>
    static void serializeArguments(Airframe * frame, std::tuple<typename std::remove_reference<Args>::type ...> & t) {
        serialize(*frame, std::get<idx>(t));
        if (idx > 0) {
            serializeArguments<idx==0?0:idx-1>(frame, t);
        }
    }
};

class RemoteAccessClient : public Endpoint {
public:
    RemoteAccessClient();

    virtual ~RemoteAccessClient();

    virtual void handleIndication(DataIndication * msg) {
        uint8_t seq;
        uint8_t status;
        msg->getAirframe() >> seq;
        msg->getAirframe() >> status;
        if (seqMap.find(seq) != seqMap.end()) {
            Callback<void(DataIndication*, RemoteAccessStatus)> cb = seqMap.at(seq);
            seqMap.erase(seq);

            cb(msg, (RemoteAccessStatus) status);
        } else {
            LOG_ERROR("Incoming message with unknown seq number");
        }
        delete msg;
    }

    virtual void handleResponse(DataResponse * msg) {
        delete msg;
    }

    template<typename... Args>
    void callRemote(std::string & module,
                                  std::string & method,
                                  uint8_t type,
                                  node_t dest,
                                  Callback<void(DataIndication*, RemoteAccessStatus)> cb,
                                  Args... args) {
        Airframe * frame = new Airframe();
        AirString entityName(method.c_str());
        AirString moduleName(method.c_str());

        // sequence 255 is used for events
        seq = (seq + 1) % 255;

        if (seqMap.find(seq) != seqMap.end()) {
            LOG_WARN("Duplicate sequence number used");
            ASSERT(false);
        }

        // serialize the parameters
        serialize(frame, args...);
        (*frame) << entityName;
        (*frame) << type;
        (*frame) << moduleName;
        (*frame) << seq;
        gateReqOut.send(new DataRequest(dest, frame, createCallback(&RemoteAccessClient::handleResponse)));
        seqMap[seq] = cb;
    }


private:
    void serialize(Airframe * frame) {
        // termination of variadic parameter expansion
    }

    template<typename T, typename... Args>
    void serialize(Airframe * frame, T val, Args... args) {
        (*frame) << val;
        serialize(frame, args...);
    }

    uint8_t seq;
    std::map<uint8_t, Callback<void(DataIndication *, RemoteAccessStatus)>> seqMap;
};

} /* namespace cometos */

#endif /* REMOTEACCESSCLIENT_H_ */
