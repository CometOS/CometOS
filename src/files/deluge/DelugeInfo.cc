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

#include "DelugeInfo.h"

using namespace cometos;

DelugeInfo::DelugeInfo(uint16_t version, uint8_t numberOfPages, file_size_t fileSize, uint8_t* pageAge, uint16_t* pageCRC, uint8_t *pageComplete) :
        mVersion(version),
        mNumberOfPages(numberOfPages),
        mFileSize(fileSize),
        pPageAge(pageAge),
        pPageCRC(pageCRC),
        pPageComplete(pageComplete) {

}

DelugeInfo::~DelugeInfo() {
    delete [] this->pPageAge;
    delete [] this->pPageCRC;
    delete [] this->pPageComplete;
}

uint16_t DelugeInfo::getVersion() const {
    return this->mVersion;
}

uint8_t DelugeInfo::getNumberOfPages() const {
    return this->mNumberOfPages;
}

file_size_t DelugeInfo::getFileSize() const {
    return this->mFileSize;
}

const uint8_t* DelugeInfo::getPageAge() const {
    return this->pPageAge;
}

const uint16_t* DelugeInfo::getPageCRC() const {
    return this->pPageCRC;
}

uint16_t DelugeInfo::getCRCVectorSize() const {
    return this->mNumberOfPages * 2;
}





void DelugeInfo::setPageCRC(uint8_t pageIndex, uint16_t crc) {
    this->pPageCRC[pageIndex] = crc;
}

uint16_t DelugeInfo::getAgeVectorSize() const {
    return DelugeInfo::GetAgeVectorSize(this->mNumberOfPages);
}

uint16_t DelugeInfo::GetAgeVectorSize(uint8_t numberOfPages) {
    return numberOfPages/2 + numberOfPages%2;
}

void DelugeInfo::SetAgeVector(uint8_t* pVector, uint8_t pageNumber, uint8_t age) {
    // Limit to nibble
    age = age & 0x0F;

    if (pageNumber % 2 == 0) {
        // Lower nibble
        pVector[pageNumber/2] = (pVector[pageNumber/2] & 0xF0) | age;
    } else {
        // Higher nibble
        pVector[pageNumber/2] = (pVector[pageNumber/2] & 0x0F) | (age << 4);
    }
}

uint8_t DelugeInfo::GetAge(uint8_t* pVector, uint8_t pageNumber) {
    if (pageNumber % 2 == 0) {
        // Lower nibble
        return pVector[pageNumber/2] & 0x0F;
    } else {
        // Higher nibble
        return (pVector[pageNumber/2] & 0xF0) >> 4;
    }
}

uint8_t DelugeInfo::getAge(uint8_t pageNumber) {
    return DelugeInfo::GetAge(this->pPageAge, pageNumber);
}

const uint8_t* DelugeInfo::getPageComplete() const {
    return this->pPageComplete;
}

uint16_t DelugeInfo::GetPageCompleteSize(uint8_t numberOfPages) {
    return (numberOfPages / 8) + ((numberOfPages%8 != 0)?1:0);
}

uint16_t DelugeInfo::getPageCompleteSize() const {
    return DelugeInfo::GetPageCompleteSize(this->mNumberOfPages);
}

bool DelugeInfo::IsPageComplete(uint8_t *pageCompleteVector, uint8_t pageIndex) {
    return pageCompleteVector[pageIndex / 8] & (1 << (pageIndex % 8));
}

uint8_t DelugeInfo::GetHighestCompletePage(uint8_t *pageCompleteVector, uint8_t numberOfPages) {
    uint8_t result = NO_PAGE_COMPLETE;
    for (uint8_t i = 0; i < numberOfPages; i++) {
        if (DelugeInfo::IsPageComplete(pageCompleteVector, i)) {
            result = i;
        } else {
            break;
        }
    }
    return result;
}

uint8_t DelugeInfo::getHighestCompletePage() {
    return DelugeInfo::GetHighestCompletePage(this->pPageComplete, this->mNumberOfPages);
}

void DelugeInfo::SetPageComplete(uint8_t *pageCompleteVector, uint8_t pageIndex, bool value) {
    if (value) {
        // Set the bit
        pageCompleteVector[pageIndex / 8] |= (1 << (pageIndex % 8));
    } else {
        // Clear the bit
        pageCompleteVector[pageIndex / 8] &= ~(1 << (pageIndex % 8));
    }
}

void DelugeInfo::setPageComplete(uint8_t pageIndex, bool value) {
    DelugeInfo::SetPageComplete(this->pPageComplete, pageIndex, value);
}







