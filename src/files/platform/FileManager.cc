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

#include "FileManager.h"

using namespace cometos;

FileManager::FileManager(const char * service_name)
: RemoteModule(service_name), verifierEvent(this,"propD")
{
}

FileManager::~FileManager()
{
}

void FileManager::initialize()
{
    RemoteModule::initialize();
    delugeHandler.setDeluge(&delugeInstance);

    simpleFileTransfer.setFileWrapper(&fileTransfer);
    verifier.setFileWrapper(&fileVerifier);

    /* Stack */
    fileRel.gateIndOut.connectTo(simpleFileTransfer.gateIndIn);
    simpleFileTransfer.gateReqOut.connectTo(fileRel.gateReqIn);
    dispatcher.gateIndOut.get(0).connectTo(fileRel.gateIndIn);
    fileRel.gateReqOut.connectTo(dispatcher.gateReqIn.get(0));

    dispatcher.gateIndOut.get(1).connectTo(delugeInstance.gateIndIn);
    delugeInstance.gateReqOut.connectTo(dispatcher.gateReqIn.get(1));

    //dispatcher.gateIndOut.get(1).connectTo(ex.gateIndIn);
    //ex.gateReqOut.connectTo(dispatcher.gateReqIn.get(1));


    /* Remote Methods */
    printDirectoryAction.setAction(CALLBACK_MET(&FileSystem::printDirectory, filesystem));
    printDirectoryAction.setArbiter(filesystem.getArbiter());
    remoteDeclare(&FileManager::printDirectory, "ls");

    generateRandomFileAction.setAction(CALLBACK_MET(&RandomFileGenerator::generateRandomFile, generator));
    generateRandomFileAction.setArbiter(generator.getArbiter());
    remoteDeclare(&FileManager::generateRandomFile, "rand");

    formatAction.setAction(CALLBACK_MET(&FileSystem::format, filesystem));
    formatAction.setArbiter(filesystem.getArbiter());
    remoteDeclare(&FileManager::format, "form");

    printFileAction.setAction(CALLBACK_MET(&FileSystem::printFile, filesystem));
    printFileAction.setArbiter(filesystem.getArbiter());
    remoteDeclare(&FileManager::printFile, "cat");

    removeAction.setAction(CALLBACK_MET(&FileSystem::remove, filesystem));
    removeAction.setArbiter(filesystem.getArbiter());
    remoteDeclare(&FileManager::remove, "rm");

    hexdumpAction.setAction(CALLBACK_MET(&FileSystem::hexdump, filesystem));
    hexdumpAction.setArbiter(filesystem.getArbiter());
    remoteDeclare(&FileManager::hexdump, "hex");

    fileTransferAction.setAction(CALLBACK_MET(&SimpleFileTransfer::run, simpleFileTransfer));
    fileTransferAction.setArbiter(simpleFileTransfer.getArbiter());
    remoteDeclare(&FileManager::transferFile, "rcp");

    fileCRCAction.setAction(CALLBACK_MET(&Verifier::getLocalFileProperties, verifier));
    fileCRCAction.setArbiter(verifier.getArbiter());
    remoteDeclare(&FileManager::getFileProperties, "prop", "propD");

    remoteDeclare(&FileManager::deluge, "deluge");
}

InputGate<DataIndication>& FileManager::getGateIndIn()
{
    return dispatcher.gateIndIn;
}

OutputGate<DataRequest>& FileManager::getGateReqOut()
{
    return dispatcher.gateReqOut;
}

/**
 * @return COMETOS_ERROR_BUSY if another operation is pending, COMETOS_PENDING otherwise
 */
cometos_error_t FileManager::printDirectory(AirString& directory)
{
    // this will correctly assert!
    // this->directory.printDirectory(directory);

    return printDirectoryAction.execute(directory,filesystem.getArbiter()->generateReleaseCallback());
} 

cometos_error_t FileManager::generateRandomFile(AirString& filename, file_size_t& size, bool& binary)
{
    return generateRandomFileAction.execute(filename, &fileGenerate, size, binary, generator.getArbiter()->generateReleaseCallback<unsigned char>());
} 

cometos_error_t FileManager::format()
{
    return formatAction.executeDelayed(filesystem.getArbiter()->generateReleaseCallback(),3000);
} 

cometos_error_t FileManager::printFile(AirString& filename)
{
    return printFileAction.execute(filename,filesystem.getArbiter()->generateReleaseCallback());
} 

cometos_error_t FileManager::remove(AirString& filename)
{
    return removeAction.execute(filename,filesystem.getArbiter()->generateReleaseCallback());
} 

cometos_error_t FileManager::hexdump(AirString& filename)
{
    return hexdumpAction.execute(filename,filesystem.getArbiter()->generateReleaseCallback());
} 

cometos_error_t FileManager::transferFile(node_t& receiver, AirString& filename)
{
    if(receiver == 0xFFFF) {
	return delugeHandler.setFile(filename, EMPTY_CALLBACK());
    }
    else {
        return fileTransferAction.execute(receiver, filename, filename, simpleFileTransfer.getArbiter()->generateReleaseCallback<cometos_error_t,node_t>());
    }
}

cometos_error_t FileManager::getFileProperties(AirString& filename)
{
    return fileCRCAction.execute(filename, CALLBACK_MET(&FileManager::filePropertiesFound,*this));
}

void FileManager::filePropertiesFound(FileProperties properties)
{
    verifierEvent.raiseEvent(properties);
    verifier.getArbiter()->release();
}

cometos_error_t FileManager::deluge(AirString& filename) {
    return delugeHandler.setFile(filename, EMPTY_CALLBACK());
}

cometos_error_t FileManager::stopDeluge() {
    delugeHandler.stop();
    return COMETOS_SUCCESS;
}











