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

#include "DelugeMaintenance.h"
#include "Deluge.h"
#include "Module.h"

using namespace cometos;

/***
 *
 */
DelugeMaintenance::DelugeMaintenance(Deluge *pDeluge) : pDeluge(pDeluge) {
    this->newRound();
}

DelugeMaintenance::~DelugeMaintenance() {
    // Remove old timers
    this->pDeluge->removeTimer();
}

void DelugeMaintenance::timer() {
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
        this->pDeluge->setTimeout(this->mRoundDuration - this->mRandomValue);
    }
}

void DelugeMaintenance::handleSummary(Airframe& frame, node_t source) {
    ASSERT(this->pDeluge->getInfo());

    // Extract summary data
    uint16_t versionNumber;
    uint8_t gamma;
    frame >> versionNumber;
    frame >> gamma;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received summary (v=" << versionNumber << ",g=" << static_cast<uint16_t>(gamma) << ")" << endl;
#endif

    // Compare summary with our summary
    uint8_t localGamma = this->pDeluge->getInfo()->getHighestCompletePage();
    if (versionNumber == this->pDeluge->getInfo()->getVersion() && gamma == localGamma) {
        // Count the amount of propagations with same version number
        this->mSameVersionPropagationAmount++;
    } else if (versionNumber < this->pDeluge->getInfo()->getVersion()) {
        // Publish our object profile
        this->sendObjectProfile();
    } else if (versionNumber == this->pDeluge->getInfo()->getVersion()
            && ((gamma > localGamma && gamma != 255) || (localGamma == 255 && gamma != 255))) {
        // Transition to RX
        return this->pDeluge->transitionToRX(source, (localGamma==255)?0:(localGamma+1));
    }
}

void DelugeMaintenance::sendObjectProfile() {
    // Create Airframe
    Airframe* frame = new Airframe();

    // Insert age vector
    uint16_t avSize = this->pDeluge->getInfo()->getAgeVectorSize();
    for (uint16_t i = 0; i < avSize; i++) {
        (*frame) << this->pDeluge->getInfo()->getPageAge()[avSize-i-1];
    }

    // Insert file size
    (*frame) << this->pDeluge->getInfo()->getFileSize();

    // Insert number of pages
    (*frame) << this->pDeluge->getInfo()->getNumberOfPages();

    // Insert version number
    (*frame) << this->pDeluge->getInfo()->getVersion();

    // Generate crc code
    uint16_t crc = 0;
    for (uint16_t i = 0; i < frame->getLength(); i++) {
        crc = crc16_update(crc, frame->getData()[i]);
    }
    (*frame) << crc;

    // Insert msg type
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::OBJECTPROFILE);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, this->pDeluge->createCallback(&Deluge::onMessageSent));
    this->pDeluge->sendRequest(req);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Objectprofile published" << endl;
#endif
}

void DelugeMaintenance::onMessageSent(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
}

void DelugeMaintenance::handleObjectProfile(Airframe& frame) {
    // Extract crc code
    uint16_t crc;
    frame >> crc;

    // Generate crc code
    uint16_t checkCRC = 0;
    for (uint16_t i = 0; i < frame.getLength(); i++) {
        checkCRC = crc16_update(checkCRC, frame.getData()[i]);
    }
    ASSERT(checkCRC == crc);

    // Extract version number
    uint16_t versionNumber;
    frame >> versionNumber;

    // Extract number of pages
    uint8_t numberOfPages;
    frame >> numberOfPages;

    // Extract file size
    file_size_t fileSize;
    frame >> fileSize;

    // Extract age vector
    uint16_t avSize = DelugeInfo::GetAgeVectorSize(numberOfPages);
    uint8_t *pAgeVector = new uint8_t[avSize];
    for (uint16_t i = 0; i < avSize; i++) {
        frame >> pAgeVector[i];
    }

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received objectprofile for version=" << static_cast<uint16_t>(versionNumber) << " and numberOfPages=" << static_cast<uint16_t>(numberOfPages) << " and filesize=" << fileSize << endl;
#endif

    // Check if the object profile is from a newer version
    if (versionNumber > this->pDeluge->getInfo()->getVersion()) {
        // Create empty CRC vector
        uint16_t crcSize = 2*numberOfPages;
        uint16_t *pCRCVector = new uint16_t[crcSize];
        memset(pCRCVector, 0, crcSize);

        // Create empty page complete vector
        uint16_t pcSize = DelugeInfo::GetPageCompleteSize(numberOfPages);
        uint8_t *pPageComplete = new uint8_t[pcSize];
        memset(pPageComplete, 0, pcSize);

        // Compare pages if version diff is less than 16
        if (versionNumber - this->pDeluge->getInfo()->getVersion() < 16) {
            // Check which pages are already complete
            for (uint8_t i = 0; i < numberOfPages; i++) {
                // Version minus age => Version since the page is same
                if (versionNumber - DelugeInfo::GetAge(pAgeVector, i) <= this->pDeluge->getInfo()->getVersion()) {
                    DelugeInfo::SetPageComplete(pPageComplete, i, true);
                }
            }
        }

        // Replace info file
        this->pDeluge->persistInfo(new DelugeInfo(versionNumber, numberOfPages, fileSize, pAgeVector, pCRCVector, pPageComplete));

        // Renew RX State
        this->refreshRX(true);

    } else {
        delete [] pAgeVector;
    }
}

void DelugeMaintenance::newRound(time_ms_t roundDuration) {
    // Check if we have to receive a page
    this->refreshRX();

    // Check if round duration is predefined
    if (roundDuration == 0) {
        this->mRoundDuration = (2*this->mRoundDuration > DELUGE_MAX_ROUND_TIME)?DELUGE_MAX_ROUND_TIME:(2*this->mRoundDuration);
    }

    // reset values
    this->mSameVersionPropagationAmount = 0;
    this->mRandomValue = (this->mRoundDuration/2) + intrand(this->mRoundDuration/2);
    this->mRoundProcessed = false;

    // Wait the duration until we try to propagate summary
    this->pDeluge->setTimeout(this->mRandomValue);
}

void DelugeMaintenance::sendSummary() {
    uint8_t gamma = this->pDeluge->getInfo()->getHighestCompletePage();

    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << gamma;
    (*frame) << static_cast<uint16_t>(this->pDeluge->getInfo()->getVersion());
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::SUMMARY);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, this->pDeluge->createCallback(&Deluge::onMessageSent));
    this->pDeluge->sendRequest(req);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Propagation of (v=" << this->pDeluge->getInfo()->getVersion() << ", g=" << static_cast<uint16_t>(gamma) << ")" << endl;
#endif
}

void DelugeMaintenance::refreshRX(bool force) {
    uint8_t gamma = this->pDeluge->getInfo()->getHighestCompletePage();
    if (gamma < this->pDeluge->getInfo()->getNumberOfPages()-1 || gamma == 255) {
        this->pDeluge->createRX((gamma==255)?0:(gamma+1), force);
    }
}









