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

#include "FileSystem.h"
#include "cometos.h"
#include "CFSArbiter.h"
#include "cfs.h"
#include "palSpi.h"
#include "palWdt.h"
//#include "PrintfApp.h"

using namespace cometos;

FileSystem::FileSystem()
{
}


void FileSystem::printDirectory(cometos::AirString name, Callback<void()> callback)
{
    getArbiter()->assertRunning();
    this->callback = callback;
    this->name = name;
    request.setCallback(CALLBACK_MET(&FileSystem::printDirGranted,*this));
    getCFSArbiter()->request(&request);
}

void FileSystem::format(Callback<void()> callback)
{
    getArbiter()->assertRunning();
    this->callback = callback;
    request.setCallback(CALLBACK_MET(&FileSystem::formatGranted,*this));
    getCFSArbiter()->request(&request);
}

void FileSystem::remove(cometos::AirString name, Callback<void()> callback)
{
    getArbiter()->assertRunning();
    this->callback = callback;
    this->name = name;
    request.setCallback(CALLBACK_MET(&FileSystem::removeGranted,*this));
    getCFSArbiter()->request(&request);
}

void FileSystem::hexdump(cometos::AirString name, Callback<void()> callback)
{
    getArbiter()->assertRunning();
    this->callback = callback;
    this->name = name;
    request.setCallback(CALLBACK_MET(&FileSystem::hexdumpGranted,*this));
    getCFSArbiter()->request(&request);
}

void FileSystem::printFile(cometos::AirString name, Callback<void()> callback)
{
    getArbiter()->assertRunning();
    this->callback = callback;
    this->name = name;
    request.setCallback(CALLBACK_MET(&FileSystem::printFileGranted,*this));
    getCFSArbiter()->request(&request);
}


void FileSystem::printDirGranted()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    if(cfs_opendir(&printDir, name.getStr()) == 0) {
        cometos::getCout() << "Content of " << name.getStr() << cometos::endl;
        flushed.setCallback(CALLBACK_MET(&FileSystem::printDirRead,*this));
        cometos::getCout().flush(&flushed);
    }
    else {
        cometos::getCout() << "Could not open " << name.getStr() << cometos::endl;
        getCFSArbiter()->release();
        this->callback();
    }
}

void FileSystem::printDirRead()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    if(cfs_readdir(&printDir, &printDirent) != -1) {
        cometos::getCout() << "File: " << printDirent.name << " (" << (file_size_t)printDirent.size << " bytes)" << cometos::endl;
        cometos::getCout().flush(&flushed);
    }
    else {
        cfs_closedir(&printDir);
        cometos::getCout() << "------" << cometos::endl;
        getCFSArbiter()->release();
        this->callback();
    }
}

void FileSystem::formatGranted()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    cometos::getCout() << "FileSystem::format" << cometos::endl;

    palWdt_pause();
    cfs_format();
    palWdt_resume();

    cometos::getCout() << "FileSystem::formatted" << cometos::endl;

    getCFSArbiter()->release();
    this->callback();
}

void FileSystem::removeGranted()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    cfs_remove(name.getStr());

    getCFSArbiter()->release();
    this->callback();
}

void FileSystem::hexdumpGranted()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    int fd = cfs_open(name.getStr(), CFS_READ);
    if(fd >= 0) {
        file_size_t fileSize = cfs_seek(fd, 0, CFS_SEEK_END);

        cometos::getCout() << "File " << name.getStr() << " of size " << fileSize << cometos::endl;

        uint8_t buf[16];

        for(file_size_t i = 0; i < fileSize; i += sizeof(buf)) {
            cfs_seek(fd, i, CFS_SEEK_SET);
            cfs_read(fd, buf, sizeof(buf));
            
            // print position
            cometos::getCout().width(7);
            cometos::getCout().fill('0');
            cometos::getCout() << cometos::hex << i << " ";

            // print values
            cometos::getCout().width(2);

            for(uint8_t j = 0; j < sizeof(buf) && i+j < fileSize; j++) {
                cometos::getCout() << (uint16_t)buf[j];
                if(j % 2 == 1) {
                    cometos::getCout() << " ";
                }
            }

            cometos::getCout() << cometos::endl;
        }
        cometos::getCout().width(0);
        cometos::getCout() << cometos::dec << cometos::endl;

        cfs_close(fd);
    }

    getCFSArbiter()->release();
    this->callback();
}

void FileSystem::printFileGranted()
{
    getArbiter()->assertRunning();
    getCFSArbiter()->assertRunning();

    int fd = cfs_open(name.getStr(), CFS_READ);
    if(fd >= 0) {
        file_size_t fileSize = cfs_seek(fd, 0, CFS_SEEK_END);

        cometos::getCout() << "File " << name.getStr() << " of size " << fileSize << cometos::endl;

        char buf[65];

        for(file_size_t i = 0; i < fileSize; i += sizeof(buf)-1) {
            cfs_seek(fd, i, CFS_SEEK_SET);
            cfs_read(fd, buf, sizeof(buf)-1);
            buf[sizeof(buf)-1] = '\0';
            cometos::getCout() << buf;
        }
        cometos::getCout() << cometos::endl;

        cfs_close(fd);
    }

    getCFSArbiter()->release();
    this->callback();
}

Arbiter* FileSystem::getArbiter()
{
    return &arbiter;
}

