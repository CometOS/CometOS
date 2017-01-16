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

#include "DelugeDataDisseminationExperiment.h"
#include "TaskScheduler.h"

Define_Module(cometos::DelugeDataDisseminationExperiment);

using namespace cometos;
using namespace omnetpp;

DelugeDataDisseminationExperiment::DelugeDataDisseminationExperiment(const char * service_name)
: Module(service_name), file(), runTransferTask(*this), findPropertiesTask(*this), initiator(false),
  filename("ddexp"), fileSize(128000), finalTask(CALLBACK_MET(&DelugeDataDisseminationExperiment::finalize,*this))
{
}

DelugeDataDisseminationExperiment::~DelugeDataDisseminationExperiment()
{
}

void DelugeDataDisseminationExperiment::initialize()
{
    Module::initialize();

    initiator = par("initiator");

    Deluge* deluge = dynamic_cast<Deluge*>(getParentModule()->getSubmodule("deluge"));
    ASSERT(deluge);
    delugeHandler.setDeluge(deluge);
    deluge->setFileCompleteCallback(CALLBACK_MET(&DelugeDataDisseminationExperiment::fileIsLoaded, *this));

    verifier = dynamic_cast<Verifier*>(getParentModule()->getSubmodule("verifier"));
    ASSERT(verifier);
    verifier->setFileWrapper(&file);

    // Find all nodes
    cModule* network = (cSimulation::getActiveSimulation())->getModuleByPath("Network");
    cModule* node = getParentModule();

    for (cModule::SubmoduleIterator i(network); !i.end(); i++)
    {
        cModule *submod = dynamic_cast<cModule*>(*i);
        if(submod->hasPar("id")) {
            int idx = submod->getIndex();
            if(idx != node->getIndex()) {
                nodes.push_back(idx);
            }
        }
    }

    // Start if initiator
    if(initiator) {
        schedule(new Message, &DelugeDataDisseminationExperiment::run, 1000);
        getScheduler().add(finalTask,1000);
    }
}

void DelugeDataDisseminationExperiment::run(Message* msg)
{
    delete msg;

    ASSERT(fileGenerator.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own fileGenerator
    fileGenerator.generateRandomFile(filename, &file, fileSize, false, CALLBACK_MET(&DelugeDataDisseminationExperiment::fileGenerated,*this));
}

void DelugeDataDisseminationExperiment::fileGenerated(cometos_error_t result)
{
    fileGenerator.getArbiter()->release();

    if(result != COMETOS_SUCCESS) {
        final(result);
        return;
    }

    getScheduler().add(findPropertiesTask);
}

void DelugeDataDisseminationExperiment::findProperties()
{
    ASSERT(verifier->getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own verifier
    verifier->getLocalFileProperties(filename, CALLBACK_MET(&DelugeDataDisseminationExperiment::localPropertiesFound,*this));
}

void DelugeDataDisseminationExperiment::localPropertiesFound(FileProperties properties)
{
    if(properties.result != COMETOS_SUCCESS) {
        final(properties.result);
        return;
    }

    localProperties = properties;
    getScheduler().add(runTransferTask);
}

void DelugeDataDisseminationExperiment::runTransfer()
{
    node_t receiver = 0xffff;

//    ASSERT(simpleFileTransfer->getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own simpleFileTransfer
//    simpleFileTransfer->run(receiver, filename, CALLBACK_MET(&DelugeDataDisseminationExperiment::transferFinished,*this));
    delugeHandler.setFile(filename, CALLBACK_MET(&DelugeDataDisseminationExperiment::transferFinished,*this));
}

void DelugeDataDisseminationExperiment::transferFinished(cometos_error_t result) //, node_t node)
{
    // IS NEVER CALLED!!!!
    //simpleFileTransfer->getArbiter()->release();
    if(result != COMETOS_SUCCESS) {
        final(result);
        return;
    }

    nodeIdx = 0;

    if(nodeIdx < nodes.size()) {
        verifier->getRemoteFileProperties(nodes[nodeIdx], filename, CALLBACK_MET(&DelugeDataDisseminationExperiment::verify,*this));
        nodeIdx++;
    }
}

void DelugeDataDisseminationExperiment::verify(node_t remote, FileProperties properties)
{
    std::cout << "Remote " << remote << std::endl;
    std::cout << "Local  Properties " << localProperties.filenameCRC << " " << localProperties.bytes << " " << localProperties.bytesNotNull << " " << localProperties.crc << std::endl;
    std::cout << "Remote Properties " << properties.filenameCRC << " " << properties.bytes << " " << properties.bytesNotNull << " " << properties.crc << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    if(nodeIdx < nodes.size()) {
        verifier->getRemoteFileProperties(nodes[nodeIdx], filename, CALLBACK_MET(&DelugeDataDisseminationExperiment::verify,*this));
        nodeIdx++;
    }
    else {
        final(COMETOS_SUCCESS);
    }
}

void DelugeDataDisseminationExperiment::final(cometos_error_t result)
{
    palExec_atomicBegin();
    //Callback<void(cometos_error_t)> tmpcb;
    //running = false;
    //tmpcb = callback;
    //callback = EMPTY_CALLBACK();
    palExec_atomicEnd();

    //if(tmpcb) {
    //    tmpcb(result);
    //}
}

void DelugeDataDisseminationExperiment::finish()
{
    std::cout << "finish " << palId_id() << std::endl;

    FileProperties properties = verifier->getLastFileProperties();
    recordScalar("filenameCRC", properties.filenameCRC);
    recordScalar("bytes", properties.bytes);
    recordScalar("bytesNotNull", properties.bytesNotNull);
    recordScalar("crc", properties.crc);

    std::cout << "Finish Properties " << properties.filenameCRC << " " << properties.bytes << " " << properties.bytesNotNull << " " << properties.crc << std::endl;
}

static bool finished = false;

void DelugeDataDisseminationExperiment::finalize() {
    if(finished) {
        std::cout << "All nodes finished!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        delugeHandler.stop();
        //exit(1);
    }
    else {
        getScheduler().add(finalTask,1000);
    }
}

void DelugeDataDisseminationExperiment::fileIsLoaded(uint16_t version, uint8_t pages) {
    static int counter = 0;
    counter++;
    if (counter >= nodes.size()) {
        finished = true;
    }
    recordScalar("timeFileReceived", simTime());
    getScheduler().add(finalTask);
}





