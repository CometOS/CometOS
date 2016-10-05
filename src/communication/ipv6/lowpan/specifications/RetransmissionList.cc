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

#include <RetransmissionList.h>
#include "LowpanAdaptionLayer.h"
#include "IPv6Request.h"
#include "palLocalTime.h"

namespace cometos_v6 {

DatagramInformation::DatagramInformation()
    : ipRequestInformation(0),
      payLoadbuffer(0),
      datagramTag(0),
      _compressedHeaderSize(0xFF),
      _sizeOfCompressedDatagram(0xFFFF),
      _status(0),
      _pointerToFirstUncompressedHeader(0),
      _sizeOfFrirstFragment(0xFF),
      _sizeOfUncompressedHeader(0xFFFF),
      _lastFragmentForThisDatagram(0),
      _rtoTick(RTO_INITIAL_VALUE >> RTO_CLOCK_MULTIPLICATION_FACTOR),
      _isRTOenabled(false),
      _isRTOTimeout(false),
      _lowpan(0) {}

DatagramInformation::~DatagramInformation() { free(false); }

void DatagramInformation::setData(IPv6Request* datagramInformation,
                                  BufferInformation* payLoadMemory,
                                  uint16_t tag, LowpanAdaptionLayer* lowpan) {
    // clear existing data
    clearFields();
    ipRequestInformation = datagramInformation;
    payLoadbuffer = payLoadMemory;
    datagramTag = tag;
    _status = LIST_STATUS_INITIALIZED;
    _lowpan = lowpan;
}

void DatagramInformation::tick() {
    if (!isFree()) {
        _status--;
        if (isFree()) {
            free(false);
        }
    }
}

void DatagramInformation::free(bool successfulTransmission) {  // TODO: ADD st
    // frame a response for the ip
    _status = LIST_STATUS_FREE;
    if (ipRequestInformation != NULL) {
        IPv6Response::ipv6ResponseCode_t success;
        if (successfulTransmission) {
            success = IPv6Response::IPV6_RC_SUCCESS;
        } else {
            success = IPv6Response::IPV6_RC_FAIL_ALREADY_COUNTED;
        }
        IPv6Response* resp = new IPv6Response(ipRequestInformation, success);
        ipRequestInformation->response(resp);
        ipRequestInformation = NULL;
    }
    if (payLoadbuffer != NULL) {
        if (payLoadbuffer->isUsed()) {
            payLoadbuffer->free();
        }
    }
    // kill the RTO timer
    _isRTOenabled = false;
}

void DatagramInformation::clearFields() {
    _compressedHeaderSize = 0xFF;
    _sizeOfCompressedDatagram = 0xFFFF;
    _sizeOfFrirstFragment = 0xFF;
    _sizeOfUncompressedHeader = 0xFFFF;
    _lastFragmentForThisDatagram = 0;
    _pointerToFirstUncompressedHeader = 0;
    resetRTOParameters();
}

void DatagramInformation::startRTOTimer() {
    // recalculate RTT
    _rtoTick = calculateRTO();
    ASSERT(_lowpan != NULL);
    _rttParameters.timeStamp = palLocalTime_get();
    _isRTOenabled = true;
}

uint16_t DatagramInformation::calculateRTO() {
    uint32_t rtoTmp = _rttParameters.RTT_VAR << RTO_K_MUL_FACTOR;
    if (rtoTmp < RTO_GRANULARITY) {
        rtoTmp = RTO_GRANULARITY;
    }
    rtoTmp = rtoTmp + _rttParameters.SRTT;
    /*   if(rtoTmp < RTO_INITIAL_VALUE){ // This is a should in the rfc
           rtoTmp = RTO_INITIAL_VALUE;
       }*/
    if (rtoTmp < RTO_GRANULARITY) {
        rtoTmp = RTO_GRANULARITY;
    }
    return (rtoTmp >> RTO_CLOCK_MULTIPLICATION_FACTOR);
}

void DatagramInformation::stopRTOTimer() {
    if (_isRTOenabled == true) {
        updateMeasuredRTTValues();
        _isRTOenabled = false;
    }
}

void DatagramInformation::updateMeasuredRTTValues() {
    if (_isRTOTimeout ==
        false) {  // Kran's alg. If we have hit the retransmission timeout
                  // before, this rtt value measured could be from a delayed
                  // normal ack response instead of from the response triggered
                  // by the retransmitted ack request
        ASSERT(_lowpan != NULL);
        time_ms_t currenttimeStamp = palLocalTime_get();
        ASSERT(currenttimeStamp >= _rttParameters.timeStamp); // >= because chance of getting a duplicate ack
        _rttParameters.timeStamp = currenttimeStamp - _rttParameters.timeStamp;
        // has the current rtt now
        // Note order of calculation is important
        _rttParameters.RTT_VAR =
            calculateRTTVAR(_rttParameters.RTT_VAR, _rttParameters.SRTT,
                            _rttParameters.timeStamp);
        _rttParameters.SRTT =
            calculateSRTT(_rttParameters.SRTT, _rttParameters.timeStamp);
    }
}

uint32_t DatagramInformation::calculateRTTVAR(uint32_t& rttVarOld,
                                              uint32_t& srttOld,
                                              time_ms_t& measuredRTT) {
    // No optimizations .. rework later
    uint32_t secondTermOfEqn;
    uint32_t firstTermOfEqn;
    if (measuredRTT > srttOld) {
        secondTermOfEqn = measuredRTT - srttOld;
    } else {
        secondTermOfEqn = srttOld - measuredRTT;
    }
    secondTermOfEqn = secondTermOfEqn >> RTO_BETA_DIV_FACTOR;
    firstTermOfEqn = rttVarOld - (rttVarOld >> RTO_BETA_DIV_FACTOR);

    return (firstTermOfEqn + secondTermOfEqn);  // check for overflow?
}

uint32_t DatagramInformation::calculateSRTT(uint32_t& srttOld,
                                            time_ms_t& measuredRTT) {
    uint32_t secondTermOfEqn = measuredRTT >> RTO_ALPHA_DIV_FACTOR;
    uint32_t firstTermOfEqn = srttOld - (srttOld >> RTO_ALPHA_DIV_FACTOR);
    return (firstTermOfEqn + secondTermOfEqn);  // check for overflow?
}

void DatagramInformation::resetRTOParameters() {
    _rttParameters.SRTT = RTO_INITIAL_VALUE;
    _rttParameters.RTT_VAR = RTO_INITIAL_VALUE >> 1;
    _rttParameters.timeStamp = 0;
    _rtoTick = RTO_INITIAL_VALUE >> RTO_CLOCK_MULTIPLICATION_FACTOR;
    _isRTOTimeout = false;
    _isRTOenabled = false;
}

bool DatagramInformation::areAllFragmentsAcked(uint32_t& ackBitmap) {
    ASSERT(hasInfo());
    ASSERT(isFree() == false);
    uint8_t sequenceNumber;
    for (sequenceNumber = 0; sequenceNumber <= _lastFragmentForThisDatagram;
         sequenceNumber++) {
        bool foundUnAckdSeqNum =
            (((ackBitmap) & (0x80000000 >> sequenceNumber)) == 0);
        if (foundUnAckdSeqNum) return false;
    }
    return true;
}

uint32_t DatagramInformation::getRetransmissionList(
    uint32_t& receivedAckBitmap) {
    uint32_t mask = ~(0x7FFFFFFF >> _lastFragmentForThisDatagram);
    uint32_t retransmissionList = ~(receivedAckBitmap) & mask;
    return retransmissionList;
}

void DatagramInformation::decrementRTOTick() {
    ASSERT(isFree() == false);
    if(_isRTOenabled == true){
        if (isRTOCounterDone()) {
            QueueObject* tmp;
            _isRTOTimeout = true;
            _isRTOenabled = false;

            uint32_t rtoAckRequestTransList = generateTransmissionListForRTO_RFRAG_AR();
            tmp = new LFFRPacket(this,
                                 rtoAckRequestTransList);  // Implicit ACK woud be
            // set inernally for
            // this transmission
            if (!_lowpan->enqueueQueueObject(tmp)) {
                startRTOTimer();  // queuing was unsuccessful. Start RTO timer so
                // that we retry (send RFRAG-AR) again
            }
        } else {
            _rtoTick--;
        }
    }
}

bool DatagramInformation::isRTOCounterDone() { return (_rtoTick <= 1); }

uint32_t DatagramInformation::generateTransmissionListForRTO_RFRAG_AR() {
    ASSERT(hasInfo());
    ASSERT( _lastFragmentForThisDatagram < 32);
    return (0x80000000 >> _lastFragmentForThisDatagram);
}

} /* namespace cometos_v6 */

