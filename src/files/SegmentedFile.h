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

#ifndef SEGMENTED_FILE_H
#define SEGMENTED_FILE_H

#include "cometos.h"
#include "cometosError.h"
#include "Callback.h"
#include "AirString.h"
#include "Arbiter.h"

namespace cometos {

typedef int16_t segment_size_t;
typedef int16_t num_segments_t;
typedef int32_t file_size_t;
typedef cometos::AirString& file_identifier_t;

class SegmentedFile {
public:

    virtual ~SegmentedFile() {

    }




    /**
     * Set the segment size
     *
	 * @param maxSegmentSize	Size of a segment in bytes (the last segment might be smaller if fileSize % maxSegmentSize != 0).
	 *                          Should be a multiple of the underlying storage segmentation (e.g. 256 bytes page size) for efficient storage access.
	 *                          Though, this is not mandatory, but ignoring it might lead to slower operation.
     *
     */
    void setMaxSegmentSize(segment_size_t maxSegmentSize)
    {
        this->maxSegmentSize = maxSegmentSize;
    }


    virtual Arbiter* getArbiter()
    {
        return &arbiter;
    }


    /**
     * Opens a file
     *
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    virtual void open(file_identifier_t fileIdentifier, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen = false) = 0;


    /**
     * Closes the current file
     *
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
     */
    virtual void close(Callback<void(cometos_error_t result)> finishedCallback) = 0;


	/**
	 * Writes to a segment
	 * 
	 * @param data			Pointer to the data to write, has to stay valid until the finishedCallback is called
	 * @param dataLength		Number of bytes in data (must be equal to getSegmentSize(segment))
	 * @param segment		The segment to write
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_FAIL if reading was not possible
	 */
	virtual void write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback) = 0;


	/**
	 * Flushes the written data. Notice that this function may take a long time depending on the memory type and the amount of written data.
	 *
	 * @param finishedCallback  Is called with COMETOS_SUCCESS after a successful flush or COMETOS_ERROR_FAIL if flushing was not possible
	 */
	virtual void flush(Callback<void(cometos_error_t result)> finishedCallback) = 0;


	/**
	 * Reads a segment
	 * 
	 * @param data			Pointer to the data to read to, has to stay valid until the finishedCallback is called
	 * @param dataLength		Number of bytes in data (must be equal to getSegmentSize(segment))
	 * @param segment		The segment to read
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_FAIL if reading was not possible
	 */
	virtual void read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, 
				     Callback<void(cometos_error_t result)> finishedCallback) = 0;


	/**
	 * Get the number of segments
	 *
	 * @return number of segments
	 */
	num_segments_t getNumSegments()
	{
	    ASSERT(maxSegmentSize > 0);
        return (getFileSize()-1)/maxSegmentSize + 1;
	}


	/**
	 * Get the segment size
	 *
	 * @return segment size in bytes
	 */
	segment_size_t getSegmentSize(num_segments_t segment)
	{
        if(segment == getNumSegments() - 1) {
            // last segment
            segment_size_t size = getFileSize() % maxSegmentSize;
            if(size > 0 || getFileSize() == 0) {
                return size;
            }
            else {
                // otherwise the last segment is a full segment
                return maxSegmentSize;
            }
        }
        else {
            return maxSegmentSize;
        }
	}


	/**
	 * Get the file size
	 *
	 * @return file size in bytes
	 */
	virtual file_size_t getFileSize() = 0;


	/**
	 * Get the maximum segment size
	 *
	 * @return maximum segment size in bytes
	 */
	segment_size_t getMaxSegmentSize()
    {
        return maxSegmentSize;
    }

    virtual bool isOpen() = 0;

private:
    segment_size_t maxSegmentSize;
    Arbiter arbiter;
};

}

#endif

