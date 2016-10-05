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

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "RemoteModule.h"
#include "AirString.h"
#include "AsyncAction.h"
#include "FSM.h"
#include "FileSystem.h"
#include "SimpleFileTransfer.h"
#include "Dispatcher.h"
#include "Verifier.h"
#include "RandomFileGenerator.h"
#include "SimpleReliabilityLayer.h"
#include "DelugeHandler.h"
#include "Deluge.h"

#define FILE_MANAGER_MODULE_NAME "fm"

namespace cometos {

class FileManager : public RemoteModule {
public:
    FileManager(const char* service_name = FILE_MANAGER_MODULE_NAME);
    ~FileManager();

    void initialize();

    cometos_error_t printDirectory(AirString& directory);
    cometos_error_t generateRandomFile(AirString& filename, file_size_t& size, bool& binary);
    cometos_error_t format();
    cometos_error_t printFile(AirString& filename);
    cometos_error_t remove(AirString& filename);
    cometos_error_t hexdump(AirString& filename);
    cometos_error_t transferFile(node_t& receiver, AirString& filename);
    cometos_error_t getFileProperties(AirString& filename);
    cometos_error_t deluge(AirString& filename);

    InputGate<DataIndication>& getGateIndIn();
    OutputGate<DataRequest>& getGateReqOut();

private:
    AsyncAction<AirString,Callback<void()>> printDirectoryAction;
    AsyncAction<cometos::AirString, SegmentedFile*, file_size_t, bool, Callback<void(cometos_error_t)>> generateRandomFileAction;
    DelayableAsyncAction<Callback<void()>> formatAction;
    AsyncAction<AirString,Callback<void()>> printFileAction;
    AsyncAction<AirString,Callback<void()>> removeAction;
    AsyncAction<AirString,Callback<void()>> hexdumpAction;
    AsyncAction<node_t, AirString, AirString, Callback<void(cometos_error_t,node_t)>> fileTransferAction;

    void filePropertiesFound(FileProperties properties);

    FileSystem filesystem;

    Dispatcher<2> dispatcher;

    CFSSegmentedFile fileTransfer;
    CFSSegmentedFile fileVerifier;
    CFSSegmentedFile fileGenerate;
    SimpleFileTransfer simpleFileTransfer;
    SimpleReliabilityLayer fileRel;
    RandomFileGenerator generator;
    Verifier verifier;
    RemoteEvent<FileProperties> verifierEvent;
    AsyncAction<cometos::AirString, Callback<void(FileProperties)>> fileCRCAction;

    //Example ex;

    DelugeHandler delugeHandler;
    Deluge delugeInstance;
};

}

#endif
