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

#ifndef CACHED_SEGMENTED_FILE_H
#define CACHED_SEGMENTED_FILE_H

#include "SegmentedFile.h"
#include "FSM.h"
#include "AsyncAction.h"

namespace cometos {

class CachedSegmentedFileEvent : public FSMEvent {
public:
    enum : uint8_t {
        RESULT_SIGNAL = USER_SIGNAL_START
    };

    cometos_error_t result;
};

/**
 * Caches a SegmentedFile and provides the possibility to subdivide the segments into smaller segments.
 *
 * Writes to the cached file when writeCache() is called or another segment is addressed by read or write.
 * Reads from the cached file only when another segment was previously addressed by read or write.
 */
class CachedSegmentedFile : public SegmentedFile, public FSM<CachedSegmentedFile,CachedSegmentedFileEvent> {
private:
	enum class OPERATION {
		WRITE,
		READ,
		WRITE_CACHE
	} currentOperation;

    typedef FSM<CachedSegmentedFile,CachedSegmentedFileEvent> fsm_t;

public:
    /**
	 * Initializes a CachedSegmentedFile.
	 * The object should be maintained when working with the file multiple times, since the constructor
	 * executes some extensive tasks (especially allocating memory).
     *
	 * @param maxSegmentSize	Size of a segment in bytes. Has to be a divisor of the segment size of the underlying SegmentedFile.
     */
    CachedSegmentedFile();
	~CachedSegmentedFile();

	/*
     * @param cachedFile        SegmentedFile that should be cached.
     */
	void setCachedFile(SegmentedFile* cachedFile);

    virtual bool isOpen();

    virtual void open(file_identifier_t fileIdentifier, file_size_t fileSize, Callback<void(cometos_error_t result)> finishedCallback, bool removeBeforeOpen = false);

    virtual file_size_t getFileSize();

	num_segments_t getNumSegments();

	/**
	 * Writes to a segment
	 * 
	 * @param data			Pointer to the data to write, has to stay valid until the finishedCallback is called
	 * @param dataLength		Number of bytes in data (must be equal to getSegmentSize(segment))
	 * @param segment		The segment to write
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read or COMETOS_ERROR_FAIL if reading was not possible
     *
     * @return COMETOS_ERROR_BUSY if another operation is pending, COMETOS_PENDING otherwise
	 */
	virtual void write(uint8_t* data, segment_size_t dataLength, num_segments_t segment, Callback<void(cometos_error_t result)> finishedCallback);

    /**
     * Flushes the written data
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
     *
     * @return COMETOS_ERROR_BUSY if another operation is pending, COMETOS_PENDING otherwise
	 */
	virtual void read(uint8_t* data, segment_size_t dataLength, num_segments_t segment, 
				     Callback<void(cometos_error_t result)> finishedCallback);

	/**
	 * Writes the current cache to the underlying file
	 * 
	 * @param finishedCallback	Is called with COMETOS_SUCCESS after a successful read, COMETOS_ERROR_BUSY if another operation is pending, COMETOS_ERROR_INVALID for argument errors or COMETOS_ERROR_FAIL if writing was not possible
	 */
    void writeCache(Callback<void(cometos_error_t result)> finishedCallback);

    void close(Callback<void(cometos_error_t result)> finishedCallback);

    virtual Arbiter* getArbiter();

private:
 	/** 
 	 * The internal handling is separated into three steps:
 	 * 1. Write to the persistent file if necessary
 	 * 2. Read from the persistent file if necessary
 	 * 3. Perform an additional operation on the cache if necessary
     * 4. Call the external callback (back in idle)
	 */

	fsmReturnStatus stateWriteIfNecessary(CachedSegmentedFileEvent& result);
	fsmReturnStatus stateReadIfNecessary(CachedSegmentedFileEvent& result);
	fsmReturnStatus stateAdditionalIfNecessary(CachedSegmentedFileEvent& result);

    fsmReturnStatus stateIdle(CachedSegmentedFileEvent& result);

    void closeAfterWrite(cometos_error_t result);

    void dispatcher(cometos_error_t result);

	void justifyCache();

	num_segments_t cachedSegment;
	num_segments_t externalSegment;
	uint8_t* externalData;
	Callback<void(cometos_error_t result)> externalCallback;
	Callback<void(cometos_error_t result)> dispatchCallback;
	Callback<void(cometos_error_t result)> closeCallback;

    uint8_t* cache;
    file_size_t cacheSize;
	SegmentedFile* cachedFile;
    bool cacheDirty;

	LoadableTask<cometos_error_t> dispatchTask;
};

}

#endif
