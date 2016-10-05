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

/**
 * @author Florian Meier
 */

#ifndef CFS_SEGMENTED_FILE_H
#define CFS_SEGMENTED_FILE_H

#include "SegmentedFile.h"
#include "AirString.h"
#include "LoadableTask.h"

namespace cometos {

class CFSSegmentedFile : public SegmentedFile {
public:
    /**
     */
	CFSSegmentedFile();

    /**
     * Opens a file
     *
     * @param filename The file name
	 * @param fileSize          Number of bytes in this file (or -1 for determining the file size automatically)
     *
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    void open(cometos::AirString& filename, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen = false);

    /**
     * Closes the current file
     *
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    void close(Callback<void(cometos_error_t result)> finishedCallback);

	/**
	 * Writes to a segment
	 * 
	 * @param data			Pointer to the data to write, has to stay valid until the finishedCallback is called
	 * @param dataLength		Number of bytes in data (must be equal to getSegmentSize(segment))
	 * @param segment		The segment to write
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_FAIL if writing was not possible
	 */
	virtual void write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Flushes the written data. This function will take a long time to execute.
     *
     * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful flush or COMETOS_ERROR_FAIL if flushing was not possible
     */
    virtual void flush(Callback<void(cometos_error_t result)> finishedCallback);

	/**
	 * Reads a segment
	 * 
	 * @param data			Pointer to the data to read to, has to stay valid until the finishedCallback is called
	 * @param dataLength		Number of bytes in data (must be equal to getSegmentSize(segment))
	 * @param segment		The segment to read
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_FAIL if reading was not possible
	 */
	virtual void read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback);


	/**
	 * Get the file size
	 *
	 * @return file size in bytes
	 */
	virtual file_size_t getFileSize();

    virtual bool isOpen();

private:
    void writeSegment(uint8_t* data, num_segments_t segment);
    void readSegment(uint8_t* data, num_segments_t segment);
    void openFile(cometos::AirString filename, bool removeBeforeOpen);
    void closeFile();
    void flushFile();

    void finish(cometos_error_t callback);

    ArbiterAction request;

    LoadableTask<uint8_t*, num_segments_t> writeSegmentTask;
    LoadableTask<uint8_t*, num_segments_t> readSegmentTask;
    LoadableTask<cometos::AirString, bool> openFileTask;
    LoadableTask<> closeFileTask;
    LoadableTask<> flushTask;

    Callback<void(cometos_error_t)> finishedCallback;

    int fd;
    file_size_t fileSize;
    bool opened;
};

}

#endif
