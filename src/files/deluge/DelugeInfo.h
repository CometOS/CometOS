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

#ifndef DELUGEINFO_H
#define DELUGEINFO_H

#include <stdint.h>

#include "DelugeConfig.h"
#include "SegmentedFile.h"

namespace cometos {

class DelugeInfo {
public:
    // The value that means no page is complete
    static const uint8_t NO_PAGE_COMPLETE = 255;
    // Calculates the age vector size
    static uint16_t GetAgeVectorSize(uint8_t numberOfPages);
    // Sets the age of an vector part
    static void SetAgeVector(uint8_t* pVector, uint8_t pageNumber, uint8_t age);
    // returns the page age
    static uint8_t GetAge(uint8_t* pVector, uint8_t pageNumber);
    // Returns the page complete vector size
    static uint16_t GetPageCompleteSize(uint8_t numberOfPages);
    // Returns true if the given page is complete
    static bool IsPageComplete(uint8_t *pageCompleteVector, uint8_t pageIndex);
    // Returns the highest bit index for complete page
    static uint8_t GetHighestCompletePage(uint8_t *pageCompleteVector, uint8_t numberOfPages);
    // Sets the given page complete
    static void SetPageComplete(uint8_t *pageCompleteVector, uint8_t pageIndex, bool value);

    // Constructor
    DelugeInfo(uint16_t version, uint8_t numberOfPages, file_size_t fileSize, uint8_t* pageAge, uint16_t* pageCRC, uint8_t* pageComplete);
    // Destructor
    ~DelugeInfo();
    // Returns the version
    uint16_t getVersion() const;
    // Returns the number of pages
    uint8_t getNumberOfPages() const;
    // Returns the file size
    file_size_t getFileSize() const;
    // returns the page age
    const uint8_t* getPageAge() const;
    // Returns the age vector size
    uint16_t getAgeVectorSize() const;
    // Returns the age of a page
    uint8_t getAge(uint8_t page);
    // returns the page crc vector
    const uint16_t* getPageCRC() const;
    // Sets the crc value of the given page
    void setPageCRC(uint8_t pageIndex, uint16_t crc);
    // Returns the crc vector size
    uint16_t getCRCVectorSize() const;
    // Returns the page complete vector
    const uint8_t* getPageComplete() const;
    // Returns the page complete vector size
    uint16_t getPageCompleteSize() const;
    // Returns the highest page complete
    uint8_t getHighestCompletePage();
    // Sets a page complete
    void setPageComplete(uint8_t pageIndex, bool value);

    // Version number
    uint16_t    mVersion = 0;
    // Number of pages
    uint8_t     mNumberOfPages = 0;
    // The filesize
    file_size_t mFileSize = 0;
    // Page age vector
    uint8_t*    pPageAge = nullptr;
    // Page crc vector
    uint16_t*   pPageCRC = nullptr;
    // Page complete vector
    uint8_t*    pPageComplete = nullptr;
};

}

#endif
