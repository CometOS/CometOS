/*
 * DelugeDataFile.cpp
 *
 *  Created on: 03.11.2016
 *      Author: ti5x15
 */

#include <DelugeDataFile.h>

#include "SegFileFactory.h"
#include "DelugeInfo.h"
#include "DelugeUtility.h"

namespace cometos {



DelugeDataFile::DelugeDataFile() : file(nullptr), currentPage(0), currentSegment(0), numPages(0), fileSize(-1) {
}

DelugeDataFile::~DelugeDataFile() {
    delete file;
    delete completePages;
}

void DelugeDataFile::open(file_identifier_t fileIdentifier, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen) {
        this->completePages = new uint8_t[DelugeInfo::GetPageCompleteSize(fileSize)];
        memset(completePages, 0xFF, DelugeInfo::GetPageCompleteSize(fileSize));

        file = SegFileFactory::CreateInstance();
        ASSERT(file->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
        file->setMaxSegmentSize(getMaxSegmentSize());
        file->open(fileIdentifier, fileSize, finishedCallback, removeBeforeOpen);
}

void DelugeDataFile::close(Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT(file);
    file->close(finishedCallback);
}

void DelugeDataFile::write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT(file);
    uint8_t page = segment / DELUGE_PACKETS_PER_PAGE;

    if (DelugeInfo::IsPageComplete(completePages, page)) {
        finishedCallback(COMETOS_ERROR_FAIL);
    } else {
        if(++currentSegment >= DelugeUtility::NumOfPacketsInPage(page, fileSize)) {
            currentSegment = 0;
            DelugeInfo::SetPageComplete(completePages, page, true);
        }
        file->write(data, file->getSegmentSize(segment), segment, finishedCallback);
    }
}

void DelugeDataFile::flush(Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT(file);
    file->flush(finishedCallback);
}

void DelugeDataFile::read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT(file);
    uint8_t page = segment / DELUGE_PACKETS_PER_PAGE;

    if(DelugeInfo::IsPageComplete(completePages, page)) {
        file->read(data, file->getSegmentSize(segment), segment, finishedCallback);
    } else {
        finishedCallback(COMETOS_ERROR_NOT_FOUND);
    }
}

file_size_t DelugeDataFile::getFileSize() {
    ASSERT(file);
    if(fileSize >= 0) {
        return fileSize;
    } else {
        return file->getFileSize();
    }
}

bool DelugeDataFile::isOpen() {
    ASSERT(file);
    return file->isOpen();
}

void DelugeDataFile::prepareUpdate(file_size_t fileSize, uint8_t *completePages, Callback<void(cometos_error_t result)> finishedCallback) {
    ASSERT(isOpen());
    this->fileSize = fileSize;
    memcpy(this->completePages, completePages, DelugeInfo::GetPageCompleteSize(fileSize));
    this->finishedCallback = finishedCallback;

    numPages = (fileSize / (file_size_t)DELUGE_PAGE_SIZE) + ((fileSize % DELUGE_PAGE_SIZE) ? 1 : 0);
    getCout() << numPages << " " << fileSize << " " << DELUGE_PAGE_SIZE << endl;

    currentPage = 0;
    currentSegment = 0;

    AirString filename("test");
    targetFile = SegFileFactory::CreateInstance();
    ASSERT(targetFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    targetFile->setMaxSegmentSize(getMaxSegmentSize());
    targetFile->open(filename, fileSize, CALLBACK_MET(&DelugeDataFile::onTargetFileOpen, *this));
}

void DelugeDataFile::onTargetFileOpen(cometos_error_t) {
    getCout() << currentPage << " " << currentSegment << " " << numPages << " " << DelugeInfo::IsPageComplete(completePages, currentPage) << endl;
    if(currentPage >= numPages) {
        file->close(CALLBACK_MET(&DelugeDataFile::finishUpdate, *this));
        return;
    }

    if(currentSegment >= DelugeUtility::NumOfPacketsInPage(currentPage, fileSize)) {
        currentPage++;
        currentSegment = 0;
    }

    if(currentPage >= numPages) {
        file->close(CALLBACK_MET(&DelugeDataFile::finishUpdate, *this));
        return;
    }

    if(DelugeInfo::IsPageComplete(completePages, currentPage)) {
        file->read(buffer, getSegmentSize(currentSegment), currentPage*DELUGE_PACKETS_PER_PAGE + currentSegment, CALLBACK_MET(&DelugeDataFile::onOriginFileRead, *this));
    } else {
        currentPage++;
	currentSegment=0;
	onTargetFileOpen(COMETOS_SUCCESS);
    }
}

void DelugeDataFile::onOriginFileRead(cometos_error_t error) {
    ASSERT(error = COMETOS_SUCCESS);

    targetFile->write(buffer, getSegmentSize(currentSegment), currentPage*DELUGE_PACKETS_PER_PAGE + currentSegment, CALLBACK_MET(&DelugeDataFile::onOriginFileRead, *this));
    currentSegment++;
}

void DelugeDataFile::finishUpdate(cometos_error_t error) {
    file = targetFile;
    targetFile = nullptr;
    finishedCallback(COMETOS_SUCCESS);
}


} /* namespace cometos */
