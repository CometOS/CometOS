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

#include "RandomFileGenerator.h"

using namespace cometos;

RandomFileGenerator::RandomFileGenerator()
: genRandActive(false)
{
}

Arbiter* RandomFileGenerator::getArbiter()
{
    return &arbiter;
}

void RandomFileGenerator::generateRandomFile(cometos::AirString filename, SegmentedFile* file, file_size_t size, bool binary, Callback<void(cometos_error_t)> callback)
{
    arbiter.assertRunning();

    genRandPersistentFile = file;
    genRandPersistentFile->setMaxSegmentSize(GEN_RAND_PERSISTENT_SEGMENT_SIZE);

    ASSERT(genRandFile.getArbiter()->requestImmediately() == COMETOS_SUCCESS); // we own genRandFile
    genRandFile.setCachedFile(file);
    genRandFile.setMaxSegmentSize(GEN_RAND_CACHE_SEGMENT_SIZE);

    genRandSegment = 0;
    genRandBinary = binary;
    this->callback = callback;

    genRandFile.open(filename, size, CALLBACK_MET(&RandomFileGenerator::genRandWrite,*this));
}

void RandomFileGenerator::genRandWrite(cometos_error_t result)
{
    if(result != COMETOS_SUCCESS) {
        cometos::getCout() << "Could not write to or open file while generating random file" << cometos::endl;
        genRandFile.close(CALLBACK_MET(&RandomFileGenerator::genRandFinal,*this));
        return;
    }

    // check if enough data written
    if(genRandSegment < genRandFile.getNumSegments()) {
        segment_size_t len = genRandFile.getSegmentSize(genRandSegment);

        for(segment_size_t j = 0; j < len; j++) {
            if(genRandBinary) {
                genRandBuffer[j] = intrand(256);
            }
            else {
                static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.:?!        ";
                genRandBuffer[j] = charset[intrand(sizeof(charset)-1)];
            }
        }

        genRandFile.write(genRandBuffer, len, genRandSegment, CALLBACK_MET(&RandomFileGenerator::genRandWrite,*this));

        genRandSegment++;
    }
    else {
        genRandFile.close(CALLBACK_MET(&RandomFileGenerator::genRandFinal,*this));
    }
}

void RandomFileGenerator::genRandFinal(cometos_error_t result)
{
    palExec_atomicBegin();
    Callback<void(cometos_error_t)> tmpcb;
    genRandFile.getArbiter()->release();
    tmpcb = callback;
    callback = EMPTY_CALLBACK();
    palExec_atomicEnd();

    ASSERT(tmpcb);
    tmpcb(result);
}
