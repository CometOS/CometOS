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

#include "Deluge.h"
#include "palId.h"
#include "cometosError.h"
#include "crc16.h"
#include "palWdt.h"

#include "platform/cfs/cfs.h"

using namespace cometos;

Define_Module(Deluge);

Deluge::Deluge() : fsm_t(&Deluge::stateInit), rcvdMsg(nullptr), dataFile(SegFileFactory::CreateInstance()) {
    ASSERT(dataFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
}

Deluge::~Deluge() {
}

void Deluge::initialize() {
#if 1
    palWdt_pause();
    cfs_format();
    palWdt_resume();
#endif

    //Start the state machine
   // run();
}

void Deluge::setFileCompleteCallback(Callback<void(uint16_t version, uint8_t pages)> finishedCallback) {
    this->mCallback = finishedCallback;
}

Arbiter& Deluge::getArbiter() {
    return this->mArbiter;
}

void Deluge::persistInfo(DelugeInfo *pNewInfo) {
    // Check if we have to replace info file
    if (pNewInfo) {
        delete pInfo;
        pInfo = pNewInfo;
    }

    // Store info file
    this->mInfoFile.writeInfo(pInfo, CALLBACK_MET(&Deluge::infoWritten, *this));
}

void Deluge::infoWritten(cometos_error_t error, DelugeInfo* info) {
    ASSERT(error == COMETOS_SUCCESS);
}

void Deluge::prepareForUpdate() {
    ENTER_METHOD_SILENT();
    stopTimer();
    dataFile->close(CALLBACK_MET(&Deluge::finalize, *this));
 
    // go back to the init state because a wakeup will happen here
    setStateIfIdle(&Deluge::stateMaintenance, &Deluge::stateInit);
    setStateIfIdle(&Deluge::stateMaintenance, &Deluge::stateRX);
    setStateIfIdle(&Deluge::stateMaintenance, &Deluge::stateTX);

}

void Deluge::updateDone(AirString &filename) {
    ENTER_METHOD_SILENT();
    
    this->filename = filename;	
    mActive = false;
    opened = true;
    //transition(&Deluge::stateInit);
    //newRound();
     onInfoFileLoaded(COMETOS_SUCCESS, pInfo);

    // Broadcast a wakeup message to prepare the other nodes for the update
    Airframe* frame = new Airframe();
    (*frame) << filename;
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::WAKEUP);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, this->createCallback(&Deluge::onMessageSent));
    sendRequest(req);	
}



//---------------------------------------------
//             Event generators
//---------------------------------------------

void Deluge::invoke() {
    DelugeEvent dispatchEvent;
    dispatchEvent.signal = DelugeEvent::TIMER_SIGNAL;
    dispatch(dispatchEvent);
}

void Deluge::handleIndication(DataIndication* msg) {
    if(rcvdMsg != nullptr) {
        delete rcvdMsg;
    }
    rcvdMsg = msg;

    uint8_t msgType;
    msg->getAirframe() >> msgType;

    DelugeEvent dispatchEvent;

    switch(msgType) {
    case SUMMARY:
        dispatchEvent.signal = DelugeEvent::RCV_SUMMARY_SIGNAL;
        break;
    case OBJECTPROFILE:
        dispatchEvent.signal = DelugeEvent::RCV_OBJPROFILE_SIGNAL;
        break;
    case PAGE_REQUEST:
        dispatchEvent.signal = DelugeEvent::RCV_REQUEST_SIGNAL;
        break;
    case PACKET_TRANSMISSION:
        dispatchEvent.signal = DelugeEvent::RCV_DATA_SIGNAL;
        break;
    case WAKEUP:
	dispatchEvent.signal = DelugeEvent::WAKEUP_SIGNAL;
	break;
    default:
        dispatchEvent.signal = DelugeEvent::EMPTY_SIGNAL;
    }

    dispatch(dispatchEvent);
}





//---------------------------------------------
//        Deluge State Machine
//---------------------------------------------

fsmReturnStatus Deluge::stateInit(DelugeEvent &event) {
    switch (event.signal) {
    case DelugeEvent::WAKEUP_SIGNAL:
        handleWakeup();
	return FSM_HANDLED;
    case DelugeEvent::RESULT_SIGNAL:
        dataFile->setMaxSegmentSize(DELUGE_PACKET_SEGMENT_SIZE);
        if(opened) {
	   dataFile->open(filename, -1, CALLBACK_MET(&Deluge::finalize, *this));
        } else {
	   dataFile->open(filename, DELUGE_MAX_DATAFILE_SIZE, CALLBACK_MET(&Deluge::finalize, *this), true);
	}
	return transition(&Deluge::stateMaintenance);
    default:
        return FSM_IGNORED;
    }
}

fsmReturnStatus Deluge::stateMaintenance(DelugeEvent &event) {
    switch (event.signal) {
    case DelugeEvent::ENTRY_SIGNAL:
        newRound();
        return FSM_HANDLED;
    case DelugeEvent::TIMER_SIGNAL:
        handleTimerMaintenance();
        return FSM_HANDLED;
    case DelugeEvent::RCV_SUMMARY_SIGNAL:
        return handleSummary();
    case DelugeEvent::RCV_OBJPROFILE_SIGNAL:
        handleObjectProfile();
        return FSM_HANDLED;
    case DelugeEvent::RCV_DATA_SIGNAL:
        if(mActive) {
            handlePacket();
        }
        return FSM_HANDLED;
    case DelugeEvent::RCV_REQUEST_SIGNAL:
        return handlePageRequest();
    case DelugeEvent::EXIT_SIGNAL:
        stopTimer();
        return FSM_HANDLED;
    default:
        return FSM_IGNORED;
    }
}

fsmReturnStatus Deluge::stateRX(DelugeEvent &event) {

    switch(event.signal) {
    case DelugeEvent::ENTRY_SIGNAL:
        mActive = true;
        sendPageRequest();
        startTimer(DELUGE_RX_NO_RECEIPT_DELAY);
        return FSM_HANDLED;
    case DelugeEvent::TIMER_SIGNAL:
        return handleRXTimer();
    case DelugeEvent::RCV_DATA_SIGNAL:
        handlePacket();
        return FSM_HANDLED;
    case DelugeEvent::RESULT_SIGNAL:
        return transition(&Deluge::stateMaintenance);
    case DelugeEvent::EXIT_SIGNAL:
        stopTimer();
        resetRX();
        return FSM_HANDLED;
    default:
        return FSM_IGNORED;
    }
}

fsmReturnStatus Deluge::stateTX(DelugeEvent &event) {
    switch(event.signal) {
    case DelugeEvent::ENTRY_SIGNAL:
        preparePacket();
        return FSM_HANDLED;
    case DelugeEvent::TIMER_SIGNAL:
        preparePacket();
        return FSM_HANDLED;
    case DelugeEvent::RCV_REQUEST_SIGNAL:
        handlePageRequestTX();
        return FSM_HANDLED;
    case DelugeEvent::RESULT_SIGNAL:
        return transition(&Deluge::stateMaintenance);
    case DelugeEvent::EXIT_SIGNAL:
        stopTimer();
        return FSM_HANDLED;
    default:
        return FSM_IGNORED;
    }
}





//---------------------------------------------
//         Init state helper functions
//---------------------------------------------

void Deluge::handleWakeup() {
    Airframe *frame = rcvdMsg->decapsulateAirframe();
    (*frame) >> filename;

    mInfoFile.getInfo(CALLBACK_MET(&Deluge::onInfoFileLoaded, *this));
}

void Deluge::onInfoFileLoaded(cometos_error_t result, DelugeInfo* info) {
    ASSERT(result == COMETOS_SUCCESS);

    // Store
    this->pInfo = info;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Deluge initialized" << endl;
#endif

    DelugeEvent dispatchEvent;
    dispatchEvent.signal = DelugeEvent::RESULT_SIGNAL;
    dispatch(dispatchEvent);
}





//---------------------------------------------
//     Maintenance state helper functions
//---------------------------------------------

void Deluge::newRound(time_ms_t roundDuration) {
    // Check if we have to receive a page
    uint8_t gamma = pInfo->getHighestCompletePage();
    if (gamma < pInfo->getNumberOfPages()-1 || gamma == 255) {
        uint8_t page = (gamma==255)? 0 :(gamma+1);
        if(mPageRX != page) {
            resetRX();
        }
        mPageRX = page;
        mActive = true;
    }

    // Check if round duration is predefined
    if (roundDuration == 0) {
        this->mRoundDuration = (2 * this->mRoundDuration > DELUGE_MAX_ROUND_TIME) ? DELUGE_MAX_ROUND_TIME : (2 * this->mRoundDuration);
    }

    // reset values
    this->mSameVersionPropagationAmount = 0;
    this->mRandomValue = (this->mRoundDuration / 2) + intrand(this->mRoundDuration / 2);
    this->mRoundProcessed = false;

    // Wait the duration until we try to propagate summary
    startTimer(this->mRandomValue);
}

void Deluge::handleTimerMaintenance() {
    if (this->mRoundProcessed) {
        // The last round is completed, so we start a new one
        this->newRound();
    } else {
        // Propagating summary only if same version limit is not reached
        if (this->mSameVersionPropagationAmount < DELUGE_VERSION_PROPAGATION_LIMIT) {
            this->sendSummary();
        }
        this->mRoundProcessed = true;

        // Wait the duration until round ends
        startTimer(this->mRoundDuration - this->mRandomValue);
    }
}

void Deluge::sendSummary() {
    uint8_t gamma = pInfo->getHighestCompletePage();

    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << gamma;
    (*frame) << static_cast<uint16_t>(pInfo->getVersion());
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::SUMMARY);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, this->createCallback(&Deluge::onMessageSent));
    sendRequest(req);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Propagation of (v=" << pInfo->getVersion() << ", g=" << static_cast<uint16_t>(gamma) << ")" << endl;
#endif
}

fsmReturnStatus Deluge::handleSummary() {
    ASSERT(pInfo);

    // Extract summary data
    Airframe* frame = rcvdMsg->decapsulateAirframe();
    uint16_t versionNumber;
    uint8_t gamma;
    (*frame) >> versionNumber;
    (*frame) >> gamma;
     delete frame;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received summary (v=" << versionNumber << ",g=" << static_cast<uint16_t>(gamma) << ")" << endl;
#endif

    // Compare summary with our summary
    uint8_t localGamma = pInfo->getHighestCompletePage();
    if (versionNumber == pInfo->getVersion() && gamma == localGamma) {
        // Count the amount of propagations with same version number
        this->mSameVersionPropagationAmount++;
        return FSM_HANDLED;
    } else if (versionNumber < pInfo->getVersion()) {
        // Publish our object profile
        this->sendObjectProfile();
        return FSM_HANDLED;
    } else if (versionNumber == pInfo->getVersion() && ((gamma > localGamma && gamma != 255) || (localGamma == 255 && gamma != 255))) {
        // Transition to RX
        addSuitableHost(rcvdMsg->src);
        return transition(&Deluge::stateRX);
    }
    return FSM_HANDLED;
}

void Deluge::sendObjectProfile() {
    // Create Airframe
    Airframe* frame = new Airframe();

    // Insert age vector
    uint16_t avSize = pInfo->getAgeVectorSize();
    for (uint16_t i = 0; i < avSize; i++) {
        (*frame) << pInfo->getPageAge()[avSize-i-1];
    }

    // Insert file size
    (*frame) << pInfo->getFileSize();
    // Insert number of pages
    (*frame) << pInfo->getNumberOfPages();
    // Insert version number
    (*frame) << pInfo->getVersion();

    // Generate crc code
    uint16_t crc = 0;
    for (uint16_t i = 0; i < frame->getLength(); i++) {
        crc = crc16_update(crc, frame->getData()[i]);
    }
    (*frame) << crc;

    // Insert msg type
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::OBJECTPROFILE);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, createCallback(&Deluge::onMessageSent));
    sendRequest(req);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Objectprofile published" << endl;
#endif
}

void Deluge::reopenFile(cometos_error_t error) {
    dataFile->open(filename, fileSize, CALLBACK_MET(&Deluge::finalize, *this), true);
}

void Deluge::handleObjectProfile() {
    // Extract crc code
    Airframe *frame = rcvdMsg->decapsulateAirframe();
    uint16_t crc;
    (*frame) >> crc;

    // Generate crc code
    uint16_t checkCRC = 0;
    for (uint16_t i = 0; i < frame->getLength(); i++) {
        checkCRC = crc16_update(checkCRC, frame->getData()[i]);
    }
    ASSERT(checkCRC == crc);

    // Extract version number
    uint16_t versionNumber;
    (*frame) >> versionNumber;

    // Extract number of pages
    uint8_t numberOfPages;
    (*frame) >> numberOfPages;

    file_size_t fileSize;
    (*frame) >> fileSize; 
    this->fileSize = fileSize;   
 
    //TODO handle the case that the file is a dummy TODO handle the case that file is getting resized
    //if(dataFile->getFileSize() <= 0) {
    //	dataFile->close(CALLBACK_MET(&Deluge::reopenFile, *this));
    //} 

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received objectprofile for version=" << static_cast<uint16_t>(versionNumber) << " and numberOfPages=" << static_cast<uint16_t>(numberOfPages) << " and filesize=" << fileSize << endl;
#endif

    // Check if the object profile is from a newer version
    if (versionNumber > pInfo->getVersion()) {
        // Extract age vector
        uint16_t avSize = DelugeInfo::GetAgeVectorSize(numberOfPages);
        uint8_t *pAgeVector = new uint8_t[avSize];
        for (uint16_t i = 0; i < avSize; i++) {
            (*frame) >> pAgeVector[i];
        }

        // Create empty CRC vector
        uint16_t crcSize = 2*numberOfPages;
        uint16_t *pCRCVector = new uint16_t[crcSize];
        memset(pCRCVector, 0, crcSize);

        // Create empty page complete vector
        uint16_t pcSize = DelugeInfo::GetPageCompleteSize(numberOfPages);
        uint8_t *pPageComplete = new uint8_t[pcSize];
        memset(pPageComplete, 0, pcSize);

        // Compare pages if version diff is less than 16
        if (versionNumber - pInfo->getVersion() < 16) {
            // Check which pages are already complete
            for (uint8_t i = 0; i < numberOfPages; i++) {
                // Version minus age => Version since the page is same
                if (versionNumber - DelugeInfo::GetAge(pAgeVector, i) <= pInfo->getVersion()) {
                    DelugeInfo::SetPageComplete(pPageComplete, i, true);
                }
            }
        }

        //dataFile->prepareUpdate(DELUGE_MAX_DATAFILE_SIZE, pPageComplete, CALLBACK_MET(&Deluge::finalize, *this));

        // Replace info file
        persistInfo(new DelugeInfo(versionNumber, numberOfPages, fileSize, pAgeVector, pCRCVector, pPageComplete));

        // Renew RX State
        uint8_t gamma = pInfo->getHighestCompletePage();
        if (gamma < pInfo->getNumberOfPages()-1 || gamma == 255) {
            uint8_t page = (gamma==255)?0:(gamma+1);
            resetRX();
            mPageRX = page;
            mActive = true;
        }
    }
    delete frame;
}

fsmReturnStatus Deluge::handlePageRequest() {
    uint8_t requestedPage;
    uint32_t requestedPackets;
    Airframe *frame = rcvdMsg->decapsulateAirframe();
    (*frame) >> requestedPage;
    (*frame) >> requestedPackets;
    delete frame;

    // Check availability of page
    uint8_t gamma = pInfo->getHighestCompletePage();
    if (gamma < requestedPage) {
        ASSERT(false); // this should never happen (unless the partner is evil)
    }

    mPageTX = requestedPage;
    mRequestedPackets |= requestedPackets;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] "  << __PRETTY_FUNCTION__ << ": Received page request for page " << (int)mPageTX << endl;
#endif

    return transition(&Deluge::stateTX);
}





//---------------------------------------------
//          RX state helper functions
//---------------------------------------------

void Deluge::addSuitableHost(node_t host) {
    // Check if our host memory is already occupied
    if (this->mSuitableHostIndex < DELUGE_RX_SUITABLE_HOSTS_SIZE-1 || this->mSuitableHostIndex == 255) {
        // Check if we already stored this host
        if (this->mSuitableHostIndex != 255) {
            for (uint8_t i = 0; i < this->mSuitableHostIndex; i++) {
                if (this->mSuitableHosts[i] == host) {
                    // Host is already stored so ignore this new one
                    return;
                }
            }
        }

        // Increase index
        this->mSuitableHostIndex = (this->mSuitableHostIndex==DELUGE_UINT8_OUT_OF_RAGE)?0:(this->mSuitableHostIndex+1);

        // Store host
        this->mSuitableHosts[this->mSuitableHostIndex] = host;

        // Check if this was the first host
        if (this->mCurrentHostIndex == DELUGE_UINT8_OUT_OF_RAGE) {
            this->mCurrentHostIndex = 0;
        }
    }
}

fsmReturnStatus Deluge::handleRXTimer() {
    if (this->mPacketsSinceLastTimer == 0) {
        // We dont receive data
        if (this->mCurrentHostIndex < this->mSuitableHostIndex) {
            // change host and send page request to it
            this->mCurrentHostIndex++;
            this->sendPageRequest();
            return FSM_HANDLED;
        } else {
            // No available host, leave RX
            return transition(&Deluge::stateMaintenance);
        }
    }

    // Reset timer
    this->mPacketsSinceLastTimer = 0;
    startTimer(DELUGE_RX_NO_RECEIPT_DELAY);
    return FSM_HANDLED;
}

void Deluge::sendPageRequest() {
    uint8_t gamma = pInfo->getHighestCompletePage();
    if (gamma < pInfo->getNumberOfPages()-1 || gamma == 255) {
        mPageRX = (gamma==255)?0:(gamma+1);
    }

    // Reset the packets received until now
    mPacketsSinceLastTimer = 0;

    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << static_cast<uint32_t>(this->mPacketsMissing);
    (*frame) << static_cast<uint8_t>(this->mPageRX);
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::PAGE_REQUEST);

#ifdef DELUGE_OUTPUT
        getCout() << "[" << palId_id() << "] "  << __PRETTY_FUNCTION__ << ": Requesting page " << (int)mPageRX << endl;
#endif

    // Send broadcast msg
    ASSERT(this->mCurrentHostIndex < DELUGE_UINT8_OUT_OF_RAGE);
    DataRequest* req = new DataRequest(mSuitableHosts[this->mCurrentHostIndex], frame, createCallback(&Deluge::onMessageSent));
    sendRequest(req);
}

void Deluge::handlePacket() {
    if (this->mPageCheckActive)
        return;

    // Store that we received a packet
    this->mPacketsSinceLastTimer++;

    // Extract data
    uint8_t page;
    uint8_t packet;
    uint16_t crc;
    Airframe *frame = rcvdMsg->decapsulateAirframe();
    this->mBuffer.clear();
    (*frame) >> page;
    (*frame) >> this->mPageCRC;
    (*frame) >> packet;
    (*frame) >> crc;
    (*frame) >> this->mBuffer;
    delete frame;

    if (page != this->mPageRX) {
#ifdef DELUGE_OUTPUT
        getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received page " << (int)page << " is not expected" << endl;
#endif
        return;
    }

    // Calculate own crc and compare to received
    uint16_t localCRC = Verifier::updateCRC(0, this->mBuffer.getBuffer(), this->mBuffer.getSize());
    if (localCRC != crc) {
        getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received invalid Packet" << endl;

        // Request all required packets again
        return this->sendPageRequest();
    }

    // Remove packet from required packets
    DelugeUtility::UnsetBit(&this->mPacketsMissing, packet);

    if (DelugeUtility::GetLeastSignificantBitSet(this->mPacketsMissing) >=  DelugeUtility::NumOfPacketsInPage(mPageRX, pInfo->getFileSize())) {
       // getCout() << "start page check mPacketsMissing 0x" << hex << mPacketsMissing << endl;
        this->mPageCheckActive = true;
    }

    // Store buffer into file
    uint16_t packetSize = this->dataFile->getSegmentSize(this->mPageRX * DELUGE_PACKETS_PER_PAGE + packet);
    this->mBuffer.setSize(packetSize);

    this->dataFile->write(this->mBuffer.getBuffer(), packetSize, this->mPageRX * DELUGE_PACKETS_PER_PAGE + packet, CALLBACK_MET(&Deluge::onPacketWritten, *this));

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received packet=" << static_cast<uint16_t>(packet) << " with size=" << static_cast<uint16_t>(this->mBuffer.getSize()) << endl;
#endif
}

void Deluge::onPacketWritten(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
     // Check if we are done
     if (DelugeUtility::GetLeastSignificantBitSet(this->mPacketsMissing) >= DelugeUtility::NumOfPacketsInPage(this->mPageRX, pInfo->getFileSize())) {
         // reset the needed packets
         uint16_t numPacketsInPage = DelugeUtility::NumOfPacketsInPage((mPageRX+1)%pInfo->getNumberOfPages(), pInfo->getFileSize());
         DelugeUtility::SetFirstBits(&this->mPacketsMissing, numPacketsInPage);

         mPageCheckPacket = 0;
         mPageCheckCRC = 0;

 #ifdef DELUGE_OUTPUT
         getCout() << "[" << palId_id() << "] "  << __PRETTY_FUNCTION__ << ": Start page check" << endl;
 #endif

         uint16_t packetSize = this->dataFile->getSegmentSize(this->mPageRX * DELUGE_PACKETS_PER_PAGE);
         this->mBuffer.setSize(packetSize);
         this->dataFile->read(this->mBuffer.getBuffer(), packetSize, this->mPageRX * DELUGE_PACKETS_PER_PAGE, CALLBACK_MET(&Deluge::onPageCheck, *this));

     }
}

void Deluge::onPageCheck(cometos_error_t result) {
    // Set buffer size, may be important for last packet because this can be smaller
    this->mBuffer.setSize(DelugeUtility::PacketSize(this->mPageRX, this->mPageCheckPacket, pInfo->getFileSize()));

    // Store CRC value
    this->mPageCheckCRC = Verifier::updateCRC(this->mPageCheckCRC, this->mBuffer.getBuffer(), this->mBuffer.getSize());

    // Increase packet index
    this->mPageCheckPacket++;

    if (this->mPageCheckPacket >= DelugeUtility::NumOfPacketsInPage(this->mPageRX, pInfo->getFileSize())) {
        // Check CRC from our check and the remote CRC        mPacketsMissing = 0;

        if (this->mPageCheckCRC != this->mPageCRC) {
#ifdef DELUGE_OUTPUT
            getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received invalid page=" << dec << static_cast<uint16_t>(this->mPageRX) << endl;
#endif
            // Remove our instance, then the request process is started again
            DelugeEvent dispatchEvent;
            dispatchEvent.signal = DelugeEvent::RESULT_SIGNAL;
            dispatch(dispatchEvent);
            return;
        }

        // Set page complete
        pInfo->setPageComplete(this->mPageRX, true);
        // Set crc for page
        pInfo->setPageCRC(this->mPageRX, this->mPageCheckCRC);
        // Persist the new version of info file and the main deluge class recognizes that it has to start maintenance again
        persistInfo();

        // Check if we completely loaded the file
        bool finished = false;
        if (pInfo->getVersion() != 0 && pInfo->getNumberOfPages()-1 == pInfo->getHighestCompletePage()) {
            //pDeluge->recordScalar("timeFileReceived", simTime());
            finished = true;
        }

#ifdef DELUGE_OUTPUT
        getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": RX for page " << static_cast<uint16_t>(this->mPageRX) << " done" << endl;
#endif

        if(finished) {
            if (this->mCallback) {
                this->mCallback(pInfo->getVersion(), pInfo->getNumberOfPages());
            }
        }

        static uint8_t counter = 0;
        if(counter++ == DELUGE_PAGES_BEFORE_FLUSH) {
              dataFile->flush(CALLBACK_MET(&Deluge::finalize, *this));
              counter = 0;
        }

        DelugeEvent dispatchEvent;
        dispatchEvent.signal = DelugeEvent::RESULT_SIGNAL;
        dispatch(dispatchEvent);

    } else {
        // Check if we have all packets checked
        int16_t segment = this->mPageRX * DELUGE_PACKETS_PER_PAGE + this->mPageCheckPacket;
        uint16_t packetSize = this->dataFile->getSegmentSize(segment);
        this->mBuffer.setSize(packetSize);
        this->dataFile->read(this->mBuffer.getBuffer(), packetSize, segment, CALLBACK_MET(&Deluge::onPageCheck, *this));
    }
}

void Deluge::resetRX() {
    memset(this->mSuitableHosts, 0, DELUGE_RX_SUITABLE_HOSTS_SIZE*sizeof(node_t));
    mSuitableHostIndex = 255;
    mCurrentHostIndex = 255;
    mPageCheckActive = false;
    mActive = false;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] "  << __PRETTY_FUNCTION__ << "stopped RX" << endl;
#endif
}






//---------------------------------------------
//          TX state helper functions
//---------------------------------------------

void Deluge::preparePacket() {
    // Remove last packet to send
    if (this->mPacketToSend != DELUGE_UINT8_OUT_OF_RAGE) {
        DelugeUtility::UnsetBit(&this->mRequestedPackets, this->mPacketToSend);
    }

    // Get lowest index
    this->mPacketToSend = DelugeUtility::GetLeastSignificantBitSet(this->mRequestedPackets);
    // Check if we have to send packets
    uint16_t segment = (uint16_t)this->mPageTX * DELUGE_PACKETS_PER_PAGE + mPacketToSend;
    if (this->mPacketToSend >= DELUGE_PACKETS_PER_PAGE || segment >= this->dataFile->getNumSegments()) {
        mPacketToSend = 255;
        mRequestedPackets = 0;

        DelugeEvent dispatchEvent;
        dispatchEvent.signal = DelugeEvent::RESULT_SIGNAL;
        dispatch(dispatchEvent);
        return;
    }

    // Set buffer size
    this->mBuffer.setSize(this->dataFile->getSegmentSize(this->mPageTX * DELUGE_PACKETS_PER_PAGE + mPacketToSend));

    // Open segment/packet
    this->dataFile->read(this->mBuffer.getBuffer(), this->dataFile->getSegmentSize(segment), segment, CALLBACK_MET(&Deluge::sendPacket,*this));
}

void Deluge::sendPacket(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Set buffer size, may be important for last packet because this can be smaller
    this->mBuffer.setSize(DelugeUtility::PacketSize(this->mPageTX, this->mPacketToSend, pInfo->getFileSize()));

    ASSERT(this->mBuffer.getSize() > 0);

    // Calculate buffer crc code
    uint16_t crc = Verifier::updateCRC(0, this->mBuffer.getBuffer(), this->mBuffer.getSize());

    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << this->mBuffer;
    (*frame) << static_cast<uint16_t>(crc);
    (*frame) << static_cast<uint8_t>(this->mPacketToSend);
    (*frame) << static_cast<uint16_t>(pInfo->getPageCRC()[this->mPageTX]);
    (*frame) << static_cast<uint8_t>(this->mPageTX);
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::PACKET_TRANSMISSION);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, createCallback(&Deluge::onMessageSent));
    sendRequest(req);

    // Set timeout
    startTimer(DELUGE_TX_SEND_DELAY);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Transmitted packet " << static_cast<uint16_t>(this->mPacketToSend) << endl;
#endif
}

void Deluge::handlePageRequestTX() {
    Airframe* frame = rcvdMsg->decapsulateAirframe();
    ASSERT(frame != nullptr);
    uint8_t requestedPage;
    uint32_t requestedPackets;
    (*frame) >> requestedPage;
    (*frame) >> requestedPackets;
    delete frame;

    if(mPageTX == requestedPage) {
        mRequestedPackets |= requestedPackets;
    }
}





//---------------------------------------------
//          common helper functions
//---------------------------------------------

void Deluge::finalize(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
}

void Deluge::onMessageSent(DataResponse* resp) {
    delete resp;
}

void Deluge::startTimer(time_ms_t ms) {
    getScheduler().add(*this, ms);
}

void Deluge::stopTimer() {
    getScheduler().remove(*this);
}
