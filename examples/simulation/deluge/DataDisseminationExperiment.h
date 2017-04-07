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

#include "Module.h"
#include "SimpleFileTransfer.h"
#include "Dispatcher.h"
#include "RAMSegmentedFile.h"
#include "RandomFileGenerator.h"
#include "Verifier.h"

#define DATA_DISSEMINATION_EXPERIMENT_MODULE_NAME "dexp"

namespace cometos {

class DataDisseminationExperiment : public Module {
public:
    DataDisseminationExperiment(const char* service_name = DATA_DISSEMINATION_EXPERIMENT_MODULE_NAME);
    ~DataDisseminationExperiment();

    void initialize();
    void finish();

    void run(Message* msg);

private:
    void fileGenerated(cometos_error_t result);
    void findProperties();
    void localPropertiesFound(FileProperties properties);
    void runTransfer();
    void transferFinished(cometos_error_t result, node_t node);
    void verify(node_t remote, FileProperties properties);
    void final(cometos_error_t result);

    RAMSegmentedFile file;
    RandomFileGenerator fileGenerator;
    BoundedTask<DataDisseminationExperiment,&DataDisseminationExperiment::runTransfer> runTransferTask;
    BoundedTask<DataDisseminationExperiment,&DataDisseminationExperiment::findProperties> findPropertiesTask;
    bool initiator;
    SimpleFileTransfer* simpleFileTransfer;
    Verifier* verifier;
    AirString filename;
    file_size_t fileSize;

    std::vector<node_t> nodes;
    uint16_t nodeIdx;

    FileProperties localProperties;
};

}

