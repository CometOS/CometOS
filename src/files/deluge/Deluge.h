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

#ifndef DELUGE_H
#define DELUGE_H

#include "FSM.h"
#include "Endpoint.h"
#include "Arbiter.h"
#include "Task.h"
#include "Verifier.h"

#include "DelugeInfo.h"
#include "DelugeInfoFile.h"
#include "DelugeConfig.h"
#include "DelugeUtility.h"
#include "DelugeDataFile.h"

namespace cometos {


// Event signals for the Deluge state machine
class DelugeEvent : public FSMEvent {
public:
    enum : uint8_t {
        RESULT_SIGNAL = USER_SIGNAL_START,
        TIMER_SIGNAL,
        RCV_SUMMARY_SIGNAL,
        RCV_OBJPROFILE_SIGNAL,
        RCV_REQUEST_SIGNAL,
        RCV_DATA_SIGNAL,
	WAKEUP_SIGNAL
    };
};

class Deluge : public Endpoint, public FSM<Deluge, DelugeEvent>, public Task {
public:
    // Different types of messages for the deluge algorithm
    enum MessageType {SUMMARY = 0, OBJECTPROFILE = 1, PAGE_REQUEST = 2, PACKET_TRANSMISSION = 3, WAKEUP = 4};

    // Constructor
    Deluge();
    ~Deluge();

    // Initialize the Deluge algorithm and start the state machine
    virtual void initialize() override;

    // Called when the timer expires
    virtual void invoke() override;
    // Called when a message is received
    virtual void handleIndication(DataIndication* msg) override;


    // set file complete callback
    void setFileCompleteCallback(Callback<void(uint16_t version, uint8_t pages)> finishedCallback);

    // Returns the deluge arbiter
    Arbiter& getArbiter();
    // prepare for manual update
    void prepareForUpdate();
    // Signal that the update of given file is done
    void updateDone(AirString &filename);
    // Persists the info file
    void persistInfo(DelugeInfo *pNewInfo = nullptr);

private:
    typedef FSM<Deluge, DelugeEvent> fsm_t;

    fsmReturnStatus stateInit(DelugeEvent &event);
      void handleWakeup();
      void onInfoFileLoaded(cometos_error_t result, DelugeInfo* info);

    fsmReturnStatus stateMaintenance(DelugeEvent &event);
      // starts a new round and computes time to send summary
      void newRound(time_ms_t roundDuration = 0);
      // propagates the own summary if necessary
      void handleTimerMaintenance();
      // sends the own summary
      void sendSummary();
      // handles the summary received from another node
      fsmReturnStatus handleSummary();
      // sends the own object profile
      void sendObjectProfile();
      // handles the object profile from another node
      void handleObjectProfile();
      void ready(cometos_error_t result);
      void reopenFile(cometos_error_t error);
      // handles a page request from another node and transitions to RX
      fsmReturnStatus handlePageRequest();

    fsmReturnStatus stateRX(DelugeEvent &event);
      // adds a suitable host for the page request
      void addSuitableHost(node_t host);
      // handles a connection timeout
      fsmReturnStatus handleRXTimer();
      // sends a page request to a suitable node
      void sendPageRequest();
      // receives a data packet
      void handlePacket();
      // checks if all packets of a page are received
      void onPacketWritten(cometos_error_t result);
      // checks the crc checksum of the page
      void onPageCheck(cometos_error_t result);
      // reset all state variables of the RX state
      void resetRX() ;

    fsmReturnStatus stateTX(DelugeEvent &event);
      // prepares a packet for broadcasting
      void preparePacket();
      // sends the prepared packet
      void sendPacket(cometos_error_t result);
      // adds the requested packet to the packets to send if possible
      void handlePageRequestTX();

    // called by multiple functions as a callback to finalize the current operation
    void finalize(cometos_error_t result);
    // callback after a message was successfully sent
    void onMessageSent(DataResponse* resp);

    // starts a timer for the given amount of ms
    void startTimer(time_ms_t ms);
    // stops the timer
    void stopTimer();

    // Callback when info file is persisted
    void infoWritten(cometos_error_t error, DelugeInfo* info);


    // The callback that is used if the file is completely loaded
    Callback<void(uint16_t version, uint8_t pages)> mCallback;

    // FOR MAINTENANCE
    // number of propagations with the same version
    uint16_t mSameVersionPropagationAmount = 0;
    // was the summary already sent ?
    bool mRoundProcessed = false;
    // random amount of ms to wait before propagating the summary
    time_ms_t mRandomValue = 0;
    // duration of a round in ms
    time_ms_t mRoundDuration = DELUGE_MIN_ROUND_TIME;


    //FOR RX
    // array of all suitable hosts for a page request
    node_t mSuitableHosts[DELUGE_RX_SUITABLE_HOSTS_SIZE];
    uint8_t mSuitableHostIndex = 255;
    uint8_t mCurrentHostIndex = 255;
    // the page that is requested
    uint8_t mPageRX = 0;
    // the packets that are still missing
    uint32_t mPacketsMissing = 0xFFFFFFFF;
    // the crc16 of the page
    uint16_t mPageCRC = 0;
    // the crc16 of the page computed at the node
    uint16_t mPageCheckCRC = 0;
    // the packet to check
    uint8_t mPageCheckPacket = 0;
    bool mPageCheckActive = false;
    uint8_t mPacketsSinceLastTimer = 0;
    // is RX active ?
    bool mActive = false;

    // FOR TX
    // the packet to send next
    uint8_t mPacketToSend = 255;
    // the requested packets (every bit is one packet)
    uint32_t mRequestedPackets = 0;
    // the page to send
    uint8_t mPageTX = 0;


    Vector<uint8_t, DELUGE_PACKET_SEGMENT_SIZE> mBuffer;

    // The deluge arbiter
    Arbiter mArbiter;
    // the deluge info of the node
    DelugeInfo *pInfo;
    // The file the info is written to
    DelugeInfoFile mInfoFile;
    // the last received message
    DataIndication *rcvdMsg;
    // the data file of the deluge algorithm
    SegmentedFile *dataFile;
    // the destination filename
    AirString filename;
    file_size_t fileSize;
    bool opened = false;
};

}

#endif
