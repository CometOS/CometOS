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

#include "DelugeRX.h"
#include "Deluge.h"
#include "DelugeUtility.h"
#include "Verifier.h"

using namespace cometos;

/***
 *
 */
DelugeRX::DelugeRX(Deluge *pDeluge, uint8_t page) :
        pFile(SegFileFactory::CreateInstance()),
        mFilename(DELUGE_DATA_FILE),
        pDeluge(pDeluge),
        mPage(page)
{
    // Open file
    ASSERT(this->pFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    this->pFile->setMaxSegmentSize(DELUGE_PACKET_SEGMENT_SIZE);
    this->pFile->open(this->mFilename, DELUGE_MAX_DATAFILE_SIZE, CALLBACK_MET(&DelugeRX::onFileOpen, *this));

    // Calculate missing packets
    this->mPacketsMissing = 0;
    uint16_t numPacketsInPage = DelugeUtility::NumOfPacketsInPage(this->mPage, this->pDeluge->getInfo()->getFileSize());
    DelugeUtility::SetFirstBits(&this->mPacketsMissing, numPacketsInPage);
    //getCout() << "constructor mPacketsMissing 0x" << hex << mPacketsMissing << " numPacketsInPage 0x" << numPacketsInPage << endl;
}

DelugeRX::~DelugeRX() {
    if (this->pFile) {
        delete this->pFile;
    }
}

void DelugeRX::timer() {
    if (this->mPacketsSinceLastTimer == 0) {
        // We dont receive data
        if (this->mCurrentHostIndex < this->mSuitableHostIndex) {
            // change host and send page request to it
            this->mCurrentHostIndex++;
            this->sendPageRequest();
        } else {
            // No available host, leave RX
            this->mActive = false;
            return this->pDeluge->deactivateRX();
        }
    }

    // Reset timer
    this->mPacketsSinceLastTimer = 0;
    this->pDeluge->setTimeout(DELUGE_RX_NO_RECEIPT_DELAY);
}

void DelugeRX::onFileOpen(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Check if the datafile is already existent
    if (this->pDeluge->getInfo()->getVersion() == 0) {
        // We have to write at the last segment in order to allocate the complete space
        this->mBuffer.setSize(this->pFile->getSegmentSize(this->pFile->getNumSegments()-1));
        memset(this->mBuffer.getBuffer(), 0x12, this->mBuffer.getSize());
        this->pFile->write(this->mBuffer.getBuffer(), this->mBuffer.getSize(), this->pFile->getNumSegments()-1, CALLBACK_MET(&DelugeRX::onLastSegmentWritten, *this));
    }
}

void DelugeRX::onLastSegmentWritten(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
}

void DelugeRX::addSuitableHost(node_t host) {
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

void DelugeRX::sendPageRequest() {
    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << static_cast<uint32_t>(this->mPacketsMissing);
    (*frame) << static_cast<uint8_t>(this->mPage);
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::PAGE_REQUEST);

    // Send broadcast msg
    DataRequest* req = new DataRequest(this->getHost(), frame, this->pDeluge->createCallback(&Deluge::onMessageSent));
    this->pDeluge->sendRequest(req);
}

void DelugeRX::activate() {
    // Set active
    this->mActive = true;

    // Sent page request
    this->sendPageRequest();

    // Set timer that checks if we continuously receive data
    this->pDeluge->setTimeout(DELUGE_RX_NO_RECEIPT_DELAY);
    this->mPacketsSinceLastTimer = 0;

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": RX active, requesting page=" << static_cast<uint16_t>(this->mPage) << endl;
#endif
}

node_t DelugeRX::getHost() const {
    ASSERT(this->mCurrentHostIndex != DELUGE_UINT8_OUT_OF_RAGE);
    return this->mSuitableHosts[this->mCurrentHostIndex];
}

void DelugeRX::onMessageSent(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
}

void DelugeRX::handlePacket(Airframe& frame) {
    if (this->mPageCheckActive)
        return;

    // Store that we received a packet
    this->mPacketsSinceLastTimer++;

    // Extract data
    uint8_t page;
    uint8_t packet;
    uint16_t crc;
    this->mBuffer.clear();
    frame >> page;
    frame >> this->mPageCRC;
    frame >> packet;
    frame >> crc;
    frame >> this->mBuffer;

    if (page != this->mPage) {
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

    if (DelugeUtility::GetLeastSignificantBitSet(this->mPacketsMissing) >= DELUGE_PACKETS_PER_PAGE) {
       // getCout() << "start page check mPacketsMissing 0x" << hex << mPacketsMissing << endl;
        this->mPageCheckActive = true;
    }

    // Store buffer into file
    uint16_t packetSize = this->pFile->getSegmentSize(this->mPage * DELUGE_PACKETS_PER_PAGE + packet);
    this->mBuffer.setSize(packetSize);

    this->pFile->write(this->mBuffer.getBuffer(), packetSize, this->mPage * DELUGE_PACKETS_PER_PAGE + packet, CALLBACK_MET(&DelugeRX::onPacketWritten, *this));

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received packet=" << static_cast<uint16_t>(packet) << " with size=" << static_cast<uint16_t>(this->mBuffer.getSize()) << endl;
#endif
}

void DelugeRX::onPacketWritten(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Check if we are done
    if (DelugeUtility::GetLeastSignificantBitSet(this->mPacketsMissing) >= DELUGE_PACKETS_PER_PAGE) {
        // start finish rx
        this->mPageCheckPacket = 0;
        this->mPageCheckCRC = 0;

#ifdef DELUGE_OUTPUT
        getCout() << "[" << palId_id() << "] "  << __PRETTY_FUNCTION__ << ": Start page check" << endl;
#endif

        uint16_t packetSize = this->pFile->getSegmentSize(this->mPage * DELUGE_PACKETS_PER_PAGE);
        this->mBuffer.setSize(packetSize);
        this->pFile->read(this->mBuffer.getBuffer(), packetSize, this->mPage * DELUGE_PACKETS_PER_PAGE, CALLBACK_MET(&DelugeRX::onPageCheck, *this));
    }
}

void DelugeRX::onPageCheck(cometos_error_t result) {
    // Set buffer size, may be important for last packet because this can be smaller
    this->mBuffer.setSize(DelugeUtility::PacketSize(this->mPage, this->mPageCheckPacket, this->pDeluge->getInfo()->getFileSize()));

    // Store CRC value
    this->mPageCheckCRC = Verifier::updateCRC(this->mPageCheckCRC, this->mBuffer.getBuffer(), this->mBuffer.getSize());

    // Increase packet index
    this->mPageCheckPacket++;

    // Check if we have all packets checked
    int16_t segment = this->mPage * DELUGE_PACKETS_PER_PAGE + this->mPageCheckPacket;
    if (this->mPageCheckPacket >= DelugeUtility::NumOfPacketsInPage(this->mPage, this->pDeluge->getInfo()->getFileSize())) {
        // Check CRC from our check and the remote CRC

        if (this->mPageCheckCRC != this->mPageCRC) {
#ifdef DELUGE_OUTPUT
            getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Received invalid page=" << dec << static_cast<uint16_t>(this->mPage) << endl;
#endif
            // Remove our instance, then the request process is started again
            return this->pDeluge->removeRX();
        }

        // Set page complete
        this->pDeluge->getInfo()->setPageComplete(this->mPage, true);
        // Set crc for page
        this->pDeluge->getInfo()->setPageCRC(this->mPage, this->mPageCheckCRC);
        // Persist the new version of info file and the main deluge class recognizes that it has to start maintenance again
        this->pDeluge->persistInfo();

        // Check if we completely loaded the file
        bool finished = false;
        if (this->pDeluge->getInfo()->getVersion() != 0 && this->pDeluge->getInfo()->getNumberOfPages()-1 == this->pDeluge->getInfo()->getHighestCompletePage()) {
            //pDeluge->recordScalar("timeFileReceived", simTime());f
            finished = true;
        }

#ifdef DELUGE_OUTPUT
        getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": RX for page " << static_cast<uint16_t>(this->mPage) << " done" << endl;
#endif

        this->pDeluge->removeRX();

        if(finished) {
            this->pDeluge->fileIsComplete();
        }

    } else {
        uint16_t packetSize = this->pFile->getSegmentSize(segment);
        this->mBuffer.setSize(packetSize);
        this->pFile->read(this->mBuffer.getBuffer(), packetSize, segment, CALLBACK_MET(&DelugeRX::onPageCheck, *this));
    }
}

uint8_t DelugeRX::getPage() const {
    return this->mPage;
}

void DelugeRX::destroy() {
    // Remove old timers
    this->pDeluge->removeTimer();

    // Close file
    this->pFile->close(CALLBACK_MET(&DelugeRX::finalize, *this));
}

void DelugeRX::finalize(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Delete ourself
    delete this;
}












