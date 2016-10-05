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

#include "DelugeTX.h"
#include "Deluge.h"
#include "DelugeUtility.h"
#include "Verifier.h"

using namespace cometos;

/***
 *
 */
DelugeTX::DelugeTX(Deluge *pDeluge, uint8_t page) :
        pDeluge(pDeluge),
        mPage(page),
        pFile(SegFileFactory::CreateInstance()),
        mFilename(DELUGE_DATA_FILE)
{
    // Open file
    ASSERT(this->pFile->getArbiter()->requestImmediately() == COMETOS_SUCCESS);
    this->pFile->setMaxSegmentSize(DELUGE_PACKET_SEGMENT_SIZE);
    this->pFile->open(this->mFilename, -1, CALLBACK_MET(&DelugeTX::preparePacket,*this));

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": TX active, transmitting page " << static_cast<uint16_t>(this->mPage) << endl;
#endif
}

DelugeTX::~DelugeTX() {
    if (this->pFile) {
        delete this->pFile;
    }
}

void DelugeTX::preparePacket(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Remove last packet to send
    if (this->mPacketToSend != DELUGE_UINT8_OUT_OF_RAGE) {
        DelugeUtility::UnsetBit(&this->mRequestedPackets, this->mPacketToSend);
    }

    // Get lowest index
    this->mPacketToSend = DelugeUtility::GetLeastSignificantBitSet(this->mRequestedPackets);

    // Check if we have to send packets
    uint16_t segment = this->mPage * DELUGE_PACKETS_PER_PAGE + mPacketToSend;
    if (this->mPacketToSend >= DELUGE_PACKETS_PER_PAGE || segment >= this->pFile->getNumSegments()) {
        this->pDeluge->removeTX();
        return;
    }

    // Set buffer size
    this->mBuffer.setSize(this->pFile->getSegmentSize(this->mPage * DELUGE_PACKETS_PER_PAGE + mPacketToSend));

    // Open segment/packet
    this->pFile->read(this->mBuffer.getBuffer(),
                     this->pFile->getSegmentSize(segment),
                     segment,
                     CALLBACK_MET(&DelugeTX::sendPacket,*this));
}

void DelugeTX::sendPacket(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Set buffer size, may be important for last packet because this can be smaller
    this->mBuffer.setSize(DelugeUtility::PacketSize(this->mPage, this->mPacketToSend, this->pDeluge->getInfo()->getFileSize()));

    ASSERT(this->mBuffer.getSize() > 0);

    // Calculate buffer crc code
    uint16_t crc = Verifier::updateCRC(0, this->mBuffer.getBuffer(), this->mBuffer.getSize());

    // Create Airframe
    Airframe* frame = new Airframe();
    (*frame) << this->mBuffer;
    (*frame) << static_cast<uint16_t>(crc);
    (*frame) << static_cast<uint8_t>(this->mPacketToSend);
    (*frame) << static_cast<uint16_t>(this->pDeluge->getInfo()->getPageCRC()[this->mPage]);
    (*frame) << static_cast<uint8_t>(this->mPage);
    (*frame) << static_cast<uint8_t>(Deluge::MessageType::PACKET_TRANSMISSION);

    // Send broadcast msg
    DataRequest* req = new DataRequest(0xFFFF, frame, this->pDeluge->createCallback(&Deluge::onMessageSent));
    this->pDeluge->sendRequest(req);

    // Set timeout
    this->pDeluge->setTimeout(DELUGE_TX_SEND_DELAY);

#ifdef DELUGE_OUTPUT
    getCout() << "[" << palId_id() << "] " << __PRETTY_FUNCTION__ << ": Transmitted packet " << static_cast<uint16_t>(this->mPacketToSend) << endl;
#endif
}

void DelugeTX::onMessageSent(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);
}

void DelugeTX::addRequestedPackets(uint32_t packets) {
    this->mRequestedPackets |= packets;
}

void DelugeTX::destroy() {
    // Remove timer
    this->pDeluge->removeTimer();

    this->pFile->close(CALLBACK_MET(&DelugeTX::finalize, *this));
}

void DelugeTX::finalize(cometos_error_t result) {
    ASSERT(result == COMETOS_SUCCESS);

    // Delete ourself
    delete this;
}

uint8_t DelugeTX::getPage() const {
    return this->mPage;
}










