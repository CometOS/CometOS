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

#include "DelugeHandler.h"

using namespace cometos;

DelugeHandler::DelugeHandler() :
        pOriginFile(SegFileFactory::CreateInstance()),
        pDestinationFile(SegFileFactory::CreateInstance()),
        mDatafilename(DELUGE_DATA_FILE) {
}

DelugeHandler::~DelugeHandler() {
    if (this->pOriginFile) {
        delete this->pOriginFile;
    }
    if (this->pDestinationFile) {
        delete this->pDestinationFile;
    }
}

void DelugeHandler::setDeluge(Deluge* deluge) {
    this->deluge = deluge;
}

cometos_error_t DelugeHandler::setFile(AirString& filename, Callback<void(cometos_error_t)> finishedCallback) {
    this->mFilename = filename;

    // Make sure we are the only one operating on this resources
    ASSERT(this->pOriginFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    ASSERT(this->pDestinationFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    ASSERT(deluge->getArbiter().requestImmediately()  == COMETOS_SUCCESS);

    // Stop all operations on deluge
    deluge->prepareForUpdate();

    // Perform delay to give deluge the time to stop processes
    cometos::getScheduler().add(*this, DELUGE_HANDLER_DELAY);

    return COMETOS_PENDING;
}

void DelugeHandler::invoke() {
    // Open origin file
    this->pOriginFile->setMaxSegmentSize(DELUGE_PACKET_SEGMENT_SIZE);
    this->pOriginFile->open(this->mFilename, -1, CALLBACK_MET(&DelugeHandler::fileOriginOpened,*this));
}

void DelugeHandler::fileOriginOpened(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Open destination file
    this->pDestinationFile->setMaxSegmentSize(DELUGE_PACKET_SEGMENT_SIZE);
    this->pDestinationFile->open(this->mDatafilename, pOriginFile->getFileSize(), CALLBACK_MET(&DelugeHandler::fileDestinationOpened,*this), true);
}

void DelugeHandler::fileDestinationOpened(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Calculate required parameters
    this->mNumberOfPages = (this->pOriginFile->getFileSize() / DELUGE_PAGE_SIZE) + ((this->pOriginFile->getFileSize() % DELUGE_PAGE_SIZE)?1:0);
    this->pPageCRC = new uint16_t[this->mNumberOfPages];
    this->mCurrentSegment = 0;

#ifdef DELUGE_OUTPUT
    getCout() << __PRETTY_FUNCTION__ << ": Opened '" << this->mFilename.getStr() << "' with size " << pOriginFile->getFileSize() << " and " << this->mNumberOfPages << " pages" << endl;
#endif

    this->readSegment();
}

void DelugeHandler::readSegment() {
    // Check if origin file is completly processed
    if (this->mCurrentSegment >= this->pOriginFile->getNumSegments()) {
            this->pOriginFile->close(CALLBACK_MET(&DelugeHandler::fileOriginClosed, *this));
            return;
    }

    // Read next segment from origin file
    this->pOriginFile->read(this->mBuffer.getBuffer(), this->pOriginFile->getSegmentSize(this->mCurrentSegment), this->mCurrentSegment, CALLBACK_MET(&DelugeHandler::segmentRead,*this));
}

void DelugeHandler::segmentRead(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Get the segment size from origin file
    uint8_t size = this->pOriginFile->getSegmentSize(this->mCurrentSegment);
    this->mBuffer.setSize(size);

    // Calculate CRC
    this->mCRC = Verifier::updateCRC(this->mCRC, this->mBuffer.getBuffer(), this->mBuffer.getSize());

    // Check page end
    if (this->mCurrentSegment % DELUGE_PACKETS_PER_PAGE == DELUGE_PACKETS_PER_PAGE - 1 || this->mCurrentSegment == this->pOriginFile->getNumSegments()-1) {
        uint8_t pageNumber = this->mCurrentSegment / DELUGE_PACKETS_PER_PAGE;
        //getCout() << "CRC for page " << cometos::dec << (uint16_t)pageNumber << " is = " << this->mCRC << endl;
        this->pPageCRC[pageNumber] = this->mCRC;
        //getCout() << "For page " << (uint16_t)pageNumber << " CRC = " << this->mCRC << endl;
        this->mCRC = 0;
    }

    // Write segment
    this->pDestinationFile->write(this->mBuffer.getBuffer(), this->pDestinationFile->getSegmentSize(this->mCurrentSegment), this->mCurrentSegment, CALLBACK_MET(&DelugeHandler::segmentWritten, *this));
}

void DelugeHandler::segmentWritten(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Next segment cycle
    this->mCurrentSegment++;

    // read next segment
    this->readSegment();
}

void DelugeHandler::fileOriginClosed(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Close destination file
    this->pDestinationFile->close(CALLBACK_MET(&DelugeHandler::finalize, *this));
}

void DelugeHandler::finalize(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Release origin and destination file
    this->pDestinationFile->getArbiter()->release();
    this->pOriginFile->getArbiter()->release();

    mInfoFile.getInfo(CALLBACK_MET(&DelugeHandler::infoFileOpened, *this));
}

void DelugeHandler::infoFileOpened(cometos_error_t result, DelugeInfo *pInfo) {
    ASSERT(result == COMETOS_SUCCESS);

    // Prepare page complete vector
    uint16_t pcvSize = DelugeInfo::GetPageCompleteSize(this->mNumberOfPages);
    uint8_t *pComplete = new uint8_t[pcvSize];
    memset(pComplete, 0xFF, pcvSize);

    if (pInfo->getVersion() == 0) {
        // Version is zero so there was no file before
        delete pInfo;
        uint8_t *pAge = new uint8_t[DelugeInfo::GetAgeVectorSize(this->mNumberOfPages)];
        for (uint8_t i = 0; i < this->mNumberOfPages; i++) {
            DelugeInfo::SetAgeVector(pAge, i, 0);
        }
        pInfo = new DelugeInfo(1, this->mNumberOfPages, this->pOriginFile->getFileSize(), pAge, this->pPageCRC, pComplete);

        getCout() << __PRETTY_FUNCTION__ << ": Created info file from scratch" << endl;
    } else {
        // Compare crc and set age vector
        uint8_t *pNewAgeVector = new uint8_t[DelugeInfo::GetAgeVectorSize(this->mNumberOfPages)];
        uint8_t equalPages = 0;
        for (uint8_t i = 0; i < this->mNumberOfPages; i++) {
            if (i < pInfo->getNumberOfPages()) {
                if (this->pPageCRC[i] != pInfo->getPageCRC()[i]) {
                    pNewAgeVector[i] = 0;
                } else {
                    uint8_t a = pInfo->getAge(i)+1;
                    DelugeInfo::SetAgeVector(pNewAgeVector, i, a);
                    equalPages++;
                }
            }
        }

        // Update info file
        uint16_t v = pInfo->getVersion();
        delete pInfo;
        pInfo = new DelugeInfo(++v, this->mNumberOfPages, this->pOriginFile->getFileSize(), pNewAgeVector, this->pPageCRC, pComplete);
#ifdef DELUGE_OUTPUT
        getCout() << __PRETTY_FUNCTION__ << ": New infofile for version=" << v << " with " << static_cast<uint16_t>(equalPages) << " aged pages" << endl;
#endif
    }

    // Persist the info object
    deluge->persistInfo(pInfo);

    // Release deluge
    deluge->getArbiter().release();

    // Start deluge again
    deluge->updateDone(mFilename);
}

void DelugeHandler::stop() {
    deluge->prepareForUpdate();
}














