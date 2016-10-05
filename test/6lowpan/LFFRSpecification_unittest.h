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

#ifndef LFFRSPECIFICATION_UNITTEST_H_
#define LFFRSPECIFICATION_UNITTEST_H_

#include "gtest/gtest.h"
#include "LFFRSpecification.h"
#include "Rfc4944Specification.h"
#include "bufferconfig.h"
#include "LowpanAdaptionLayerStructures.h"
#include "lowpan-macros.h"

namespace cometos_v6 {
class LFFRSpecificationTest : public testing::Test {
   public:
    void SetUp() {

    }
    void TearDown() {

    }
    void checkLFFRHeader();
    void testExtractLFFRHeaderInfo();

   private:

};
TEST_F(LFFRSpecificationTest, checkLFFRHeader){
    checkLFFRHeader();
}

void parseLFFRHeader(cometos::Airframe& frame,
                                              uint8_t& dGramOffset,
                                              uint16_nbo& dGramTag,
                                              uint8_t& sequenceNumber,
                                              uint16_t& dGramSize) {
    uint8_t tmp;
    frame >> dGramOffset;
    frame >> dGramTag;
    frame >>  tmp;
    sequenceNumber = (tmp >> 3) & 0x1F;
    uint8_t msbOfDatagramSize = (tmp & 0x07);
    uint8_t lsbOfDatagramSize;
    frame >> lsbOfDatagramSize;
    dGramSize = (((uint16_t)msbOfDatagramSize) << 8)| lsbOfDatagramSize;
}

void LFFRSpecificationTest::checkLFFRHeader() {
    uint8_t RFRAG = 0xE8;
    uint8_t RFRAG_AR = 0xE9;
    uint8_t RFRAG_ACK = 0xEA;
    uint8_t RFRAG_AEC = 0xEB;

    EXPECT_TRUE(isLFFRFragmentationHeader(RFRAG));
    EXPECT_TRUE(isLFFRFragmentationHeader(RFRAG_AR));
    EXPECT_TRUE(isLFFRFragmentationHeader(RFRAG_ACK));
    EXPECT_TRUE(isLFFRFragmentationHeader(RFRAG_AEC));
    EXPECT_FALSE(isLFFRFragmentationHeader(0xEC));
}

void LFFRSpecificationTest::testExtractLFFRHeaderInfo() {
    LFFRPacket packet(NULL);
    cometos::Airframe frame;
    uint16_t datagramSize = 897;
    uint16_nbo datagramTag = 90;
    uint8_t offset = 9;
    uint8_t sequenceNum = 0;

    uint16_t exdatagramSize;
    uint16_nbo exdatagramTag;
    uint8_t exoffset;
    uint8_t exsequenceNum;

    uint8_t tmp;
    packet.addLFFRHeader(frame, datagramSize, sequenceNum, datagramTag, offset);
    frame >> tmp;

    parseLFFRHeader(frame, exoffset, exdatagramTag, exsequenceNum, exdatagramSize);
    EXPECT_EQ(exoffset, offset);
    EXPECT_EQ(exdatagramTag.getUint16_t(), datagramTag.getUint16_t());
    EXPECT_EQ(exsequenceNum, sequenceNum);
    EXPECT_EQ(exdatagramSize, datagramSize);
}



}
#endif /* LFFRSPECIFICATION_UNITTEST_H_ */


