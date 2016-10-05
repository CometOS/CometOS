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

#include "DataDisseminationExperiment.h"
#include "TaskScheduler.h"
#include "palId.h"

Define_Module(cometos::DataDisseminationExperiment);

using namespace cometos;
using namespace omnetpp;

DataDisseminationExperiment::DataDisseminationExperiment(const char * service_name)
: Module(service_name), file(), runTransferTask(*this), findPropertiesTask(*this), initiator(false),
  simpleFileTransfer(NULL), filename("ddexp"), fileSize(1000)
{
}

DataDisseminationExperiment::~DataDisseminationExperiment()
{
}

void DataDisseminationExperiment::initialize()
{
    Module::initialize();

    initiator = par("initiator");

    simpleFileTransfer = dynamic_cast<SimpleFileTransfer*>(getParentModule()->getSubmodule("simpleFileTransfer"));
    ASSERT(simpleFileTransfer);
    simpleFileTransfer->setFileWrapper(&file);

    verifier = dynamic_cast<Verifier*>(getParentModule()->getSubmodule("verifier"));
    ASSERT(verifier);
    verifier->setFileWrapper(&file);

    // Find all nodes
    cModule* network = (omnetpp::cSimulation::getActiveSimulation())->getModuleByPath("Network");
    cModule* node = getParentModule();

    for (cModule::SubmoduleIterator i(network); !i.end(); i++)
    {
        cModule *submod = dynamic_cast<cModule*>(i());
        if(submod->hasPar("id")) {
            int idx = submod->getIndex();
            if(idx != node->getIndex()) {
                nodes.push_back(idx);
            }
        }
    }

    // Start if initiator
    if(initiator) {
        schedule(new Message, &DataDisseminationExperiment::run, 1000);
    }
}

void DataDisseminationExperiment::run(Message* msg)
{
    delete msg;

    ASSERT(fileGenerator.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own fileGenerator
    fileGenerator.generateRandomFile(filename, &file, fileSize, false, CALLBACK(&DataDisseminationExperiment::fileGenerated,*this));
}

void DataDisseminationExperiment::fileGenerated(cometos_error_t result)
{
    fileGenerator.getArbiter()->release();

    if(result != COMETOS_SUCCESS) {
        final(result);
        return;
    }

    getScheduler().add(findPropertiesTask);
}

void DataDisseminationExperiment::findProperties()
{
    ASSERT(verifier->getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own verifier
    verifier->getLocalFileProperties(filename, CALLBACK(&DataDisseminationExperiment::localPropertiesFound,*this));
}

void DataDisseminationExperiment::localPropertiesFound(FileProperties properties)
{
    if(properties.result != COMETOS_SUCCESS) {
        final(properties.result);
        return;
    }

    localProperties = properties;
    getScheduler().add(runTransferTask);
}

void DataDisseminationExperiment::runTransfer()
{
    node_t receiver = 0xffff;

    ASSERT(simpleFileTransfer->getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own simpleFileTransfer
    simpleFileTransfer->run(receiver, filename, filename, CALLBACK(&DataDisseminationExperiment::transferFinished,*this));
}

void DataDisseminationExperiment::transferFinished(cometos_error_t result, node_t node)
{
    simpleFileTransfer->getArbiter()->release();
    if(result != COMETOS_SUCCESS) {
        final(result);
        return;
    }

    nodeIdx = 0;

    if(nodeIdx < nodes.size()) {
        verifier->getRemoteFileProperties(nodes[nodeIdx], filename, CALLBACK(&DataDisseminationExperiment::verify,*this));
        nodeIdx++;
    }
}

void DataDisseminationExperiment::verify(node_t remote, FileProperties properties)
{
    std::cout << "Remote " << remote << std::endl;
    std::cout << "Local  Properties " << localProperties.filenameCRC << " " << localProperties.bytes << " " << localProperties.bytesNotNull << " " << localProperties.crc << std::endl;
    std::cout << "Remote Properties " << properties.filenameCRC << " " << properties.bytes << " " << properties.bytesNotNull << " " << properties.crc << std::endl;
    std::cout << "----------------------------------------------" << std::endl;

    if(nodeIdx < nodes.size()) {
        verifier->getRemoteFileProperties(nodes[nodeIdx], filename, CALLBACK(&DataDisseminationExperiment::verify,*this));
        nodeIdx++;
    }
    else {
        final(COMETOS_SUCCESS);
    }
}

void DataDisseminationExperiment::final(cometos_error_t result)
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

void DataDisseminationExperiment::finish()
{
    //std::cout << "finish " << palId_id() << std::endl;

    FileProperties properties = verifier->getLastFileProperties();
    recordScalar("filenameCRC", properties.filenameCRC);
    recordScalar("bytes", properties.bytes);
    recordScalar("bytesNotNull", properties.bytesNotNull);
    recordScalar("crc", properties.crc);

    std::cout << "Finish Properties " << properties.filenameCRC << " " << properties.bytes << " " << properties.bytesNotNull << " " << properties.crc << std::endl;
}

