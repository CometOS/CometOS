/*
 * DelugeDataFile.h
 *
 *  Created on: 03.11.2016
 *      Author: ti5x15
 */

#ifndef SRC_FILES_DELUGE_DELUGEDATAFILE_H_
#define SRC_FILES_DELUGE_DELUGEDATAFILE_H_

#include "SegmentedFile.h"
#include "DelugeConfig.h"

namespace cometos {

class DelugeDataFile : public SegmentedFile {
public:
    DelugeDataFile();
    virtual ~DelugeDataFile();

    /**
     * Opens a file
     *
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    void open(file_identifier_t fileIdentifier, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen = false);

    /**
     * Closes the current file
     *
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    void close(Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Writes to a segment. Only segments that need an update can be written. PrepareUpdate has to be called first to signal that a page needs an update.
     *
     * @param data          Pointer to the data to write, has to stay valid until the finishedCallback is called
     * @param dataLength        Number of bytes in data (must be equal to getSegmentSize(segment))
     * @param segment       The segment to write
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful write or COMETOS_ERROR_FAIL if reading was not possible
     */
    void write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Flushes the written data. Notice that this function may take a long time depending on the memory type and the amount of written data.
     *
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful flush or COMETOS_ERROR_FAIL if flushing was not possible
     */
    void flush(Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Reads a segment
     *
     * @param data          Pointer to the data to read to, has to stay valid until the finishedCallback is called
     * @param dataLength        Number of bytes in data (must be equal to getSegmentSize(segment))
     * @param segment       The segment to read
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_NOT_FOUND if the page was not transmitted yet
     *                          or COMETOS_ERROR_FAIL if reading was not possible
     */
    void read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Get the file size
     *
     * @return file size in bytes
     */
    file_size_t getFileSize();

    bool isOpen();

    /**
     * Prepares the file for an update.
     *
     * @param fileSize          The new file size in bytes
     * @param updatePages       Vector of all pages that need to be updated. Each bit represents one page. 1 -> page is up to date.
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful update or COMETOS_ERROR_FAIL if updating was not possible
     */
    void prepareUpdate(file_size_t fileSize, uint8_t *completePages, Callback<void(cometos_error_t result)> finishedCallback);

private:
    void onTargetFileOpen(cometos_error_t error);
    void onOriginFileRead(cometos_error_t error);
    void finishUpdate(cometos_error_t error);

    SegmentedFile *file;
    SegmentedFile *targetFile;
    uint8_t currentPage;
    uint8_t currentSegment;
    uint8_t numPages;

    uint8_t *completePages;
    file_size_t fileSize;
    Callback<void(cometos_error_t result)> finishedCallback;
    uint8_t buffer[DELUGE_PACKET_SEGMENT_SIZE];
};

} /* namespace cometos */

#endif /* SRC_FILES_DELUGE_DELUGEDATAFILE_H_ */
