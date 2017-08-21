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
 * @author Stefan Unterschütz
 */

#ifndef DATARESPONSE_H_
#define DATARESPONSE_H_

#include "RequestResponse.h"
#include "DataRequest.h"
#include "mac_definitions.h"

namespace cometos {

enum class DataResponseStatus {
    SUCCESS,
    NO_ACK,
    QUEUE_FULL,
    BUSY,
    CHANNEL_ACCESS_FAILURE,
    INVALID_ADDRESS,
    INVALID_PARAMETER,
    INVALID_GTS,
    EXPIRED,
    FAIL_UNKNOWN,
    NO_ROUTE
};

class DataResponse: public Response {
public:
    DataResponse(DataResponseStatus status = DataResponseStatus::SUCCESS)
    : status(status) {
    }

    DataResponse(macTxResult_t result) {
        switch(result) {
            case MTR_SUCCESS:
                status = DataResponseStatus::SUCCESS;
                break;
            case MTR_CHANNEL_ACCESS_FAIL:
                status = DataResponseStatus::CHANNEL_ACCESS_FAILURE;
                break;
            case MTR_NO_ACK:
                status = DataResponseStatus::NO_ACK;
                break;
            case MTR_INVALID:
                status = DataResponseStatus::INVALID_PARAMETER;
                break;
            default:
                ASSERT(false);
        }
    }

    DataResponseStatus status;

    bool isSuccess() const {
        return status == DataResponseStatus::SUCCESS;
    }

    bool isFailed() const {
        return status != DataResponseStatus::SUCCESS;
    }

    const char* to_str() const {
        switch(status) {
        case DataResponseStatus::SUCCESS:
            return "SUCCESS";
        case DataResponseStatus::NO_ACK:
            return "NO_ACK";
        case DataResponseStatus::QUEUE_FULL:
            return "QUEUE_FULL";
        case DataResponseStatus::BUSY:
            return "BUSY";
        case DataResponseStatus::CHANNEL_ACCESS_FAILURE:
            return "CHANNEL_ACCESS_FAILURE";
        case DataResponseStatus::INVALID_ADDRESS:
            return "INVALID_ADDRESS";
        case DataResponseStatus::INVALID_PARAMETER:
            return "INVALID_PARAMETER";
        case DataResponseStatus::INVALID_GTS:
            return "INVALID_GTS";
        case DataResponseStatus::EXPIRED:
            return "EXPIRED";
        case DataResponseStatus::FAIL_UNKNOWN:
            return "FAIL_UNKNOWN";
        case DataResponseStatus::NO_ROUTE:
            return "NO_ROUTE";
        default:
            return "";
        }
    }

private:
#ifdef OMNETPP
    std::string str() const {
        // to_str() was str() before, but str() was newly
        // introducted in OMNeT++ 5.1.1, so this can no
        // longer be used.
        return "";
    }
#endif
};


}

#endif
