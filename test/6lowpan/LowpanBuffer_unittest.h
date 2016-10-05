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

#include "gtest/gtest.h"
#include "LowpanBuffer.h"
#include "Airframe.h"
#include "omnetppDummyEnv.h"

namespace {
class LowpanBufferTest : public ::testing::Test {
public:
    static void SetUpTestCase() {
        OmnetppDummyEnv::setup();
    }

    LowpanBufferTest() :
        lowpanBuffer(1000,10)
    {}

protected:
        cometos_v6::DynLowpanBuffer lowpanBuffer;
//        cometos_v6::LowpanBuffer<1000, 10> lowpanBuffer;
};

TEST_F(LowpanBufferTest, getBufferBytewise) {
    cometos_v6::BufferInformation*     first;
    cometos_v6::BufferInformation*     second;
    ASSERT_EQ(NULL, lowpanBuffer.getBuffer(1001));
    first = lowpanBuffer.getBuffer(500);
    ASSERT_FALSE(NULL == first);
    EXPECT_EQ(500, first->getSize());
    ASSERT_EQ(NULL, lowpanBuffer.getBuffer(501));
    second = lowpanBuffer.getBuffer(500);
    ASSERT_FALSE(NULL == second);
    EXPECT_EQ(500, second->getSize());

    for (uint16_t i = 0; i < 500; i++) {
        (*first)[i] = 1;
        (*second)[i] = 2;
    }
    for (uint16_t i = 0; i < 500; i++) {
        EXPECT_EQ((*first)[i], 1);
        EXPECT_EQ((*second)[i], 2);
    }

    for (uint16_t i = 0; i < 500; i++) {
        EXPECT_EQ(first->streamOut(), 1);
        EXPECT_EQ(second->streamOut(), 2);
        EXPECT_EQ((499 - i), first->getSize());
        EXPECT_EQ((499 - i), second->getSize());
    }

    first->free();
    second->free();
    EXPECT_FALSE(first->isUsed());
    EXPECT_FALSE(second->isUsed());
}

#define TEST_PACKET_SIZE 100

TEST_F(LowpanBufferTest, getBufferBlockwise) {
    cometos_v6::BufferInformation*     first;
    cometos_v6::BufferInformation*     second;
    first = lowpanBuffer.getBuffer(500);
    ASSERT_FALSE(NULL == first);
    EXPECT_EQ(500, first->getSize());
    second = lowpanBuffer.getBuffer(500);
    ASSERT_FALSE(NULL == second);
    EXPECT_EQ(500, second->getSize());

    uint8_t testField[TEST_PACKET_SIZE];
    cometos::Airframe testFrame;
    for (uint8_t i = 0; i < TEST_PACKET_SIZE; i++) {
        testField[i] = i;
    }

    uint16_t pos = 0;
    for (uint8_t i = 0; i < 5; i++) {
        first->copyToBuffer(testField, TEST_PACKET_SIZE, (i * TEST_PACKET_SIZE));
        for (uint8_t j = 0; j < TEST_PACKET_SIZE; j++) {
            testFrame << j;
        }
        EXPECT_EQ(TEST_PACKET_SIZE, second->addFrame(testFrame, pos));
        EXPECT_EQ(pos, (i+1) * TEST_PACKET_SIZE);
    }

    for (uint8_t i = 0; i < 5; i++) {
        cometos::Airframe tmpFrame;
        tmpFrame << first->getBufferPart(TEST_PACKET_SIZE);
        EXPECT_EQ((4 - i) * TEST_PACKET_SIZE, first->getSize());
        for (uint8_t j = 0; j < TEST_PACKET_SIZE; j++) {
            uint8_t tmp;
            tmpFrame >> tmp;
            EXPECT_EQ(tmp, testField[j]);
        }
        tmpFrame << second->getBufferPart(TEST_PACKET_SIZE);
        EXPECT_EQ((4 - i) * TEST_PACKET_SIZE, second->getSize());
        for (uint8_t j = 0; j < TEST_PACKET_SIZE; j++) {
            uint8_t tmp;
            tmpFrame >> tmp;
            EXPECT_EQ(tmp, testField[TEST_PACKET_SIZE - 1 - j]);
        }
    }

    first->free();
    second->free();
    EXPECT_FALSE(first->isUsed());
    EXPECT_FALSE(second->isUsed());
}

#define TEST_FRAG_SIZE 10

TEST_F(LowpanBufferTest, getBufferFragmentation) {
    cometos_v6::BufferInformation*     a[TEST_FRAG_SIZE];
    for (uint8_t i = 0; i < TEST_FRAG_SIZE; i++) {
        a[i] = lowpanBuffer.getBuffer(TEST_PACKET_SIZE);
        ASSERT_FALSE(NULL == a[i]);
        EXPECT_EQ(TEST_PACKET_SIZE, a[i]->getSize());
        const uint8_t* tmpP = a[i]->getContent();
        a[i]->freeBegin(TEST_FRAG_SIZE);
        EXPECT_EQ(TEST_PACKET_SIZE - TEST_FRAG_SIZE, a[i]->getSize());
        EXPECT_EQ((tmpP + TEST_FRAG_SIZE), a[i]->getContent());
        a[i]->freeEnd(TEST_FRAG_SIZE);
        EXPECT_EQ(TEST_PACKET_SIZE - (2 * TEST_FRAG_SIZE), a[i]->getSize());
        EXPECT_EQ((tmpP + TEST_FRAG_SIZE), a[i]->getContent());
    }
    for (uint8_t i = 0; i < TEST_FRAG_SIZE; i++) {
        const uint8_t* tmpP = a[i]->getContent();
        a[i]->freeEnd(TEST_FRAG_SIZE);
        EXPECT_EQ(TEST_PACKET_SIZE - (3 * TEST_FRAG_SIZE), a[i]->getSize());
        EXPECT_EQ(tmpP, a[i]->getContent());
    }
    for (uint8_t i = 1; i < TEST_FRAG_SIZE; i++) {
        EXPECT_EQ((uint16_t)(a[i]->getContent() - a[i - 1]->getContent()), TEST_PACKET_SIZE - TEST_FRAG_SIZE);
    }
    for (uint8_t i = 0; i < TEST_FRAG_SIZE; i++) {
        a[i]->free();
        EXPECT_FALSE(a[i]->isUsed());
    }
}

TEST_F(LowpanBufferTest, fillBuffer) {
    cometos_v6::BufferInformation*     first;
    cometos_v6::BufferInformation*     second;
    first = lowpanBuffer.getBuffer(1000);
    ASSERT_FALSE(NULL == first);
    EXPECT_EQ(1000, first->getSize());
    second = lowpanBuffer.getBuffer(1);
    ASSERT_TRUE(NULL == second);
    first->freeEnd(1);
    EXPECT_EQ(999, first->getSize());
    second = lowpanBuffer.getBuffer(1);
    EXPECT_FALSE(NULL == second);
    EXPECT_EQ(1, second->getSize());
    first->free();
    second->free();
    EXPECT_FALSE(first->isUsed());
    EXPECT_FALSE(second->isUsed());
}

TEST_F(LowpanBufferTest, populateFrame){
    cometos_v6::BufferInformation* spaceForData;
    uint8_t dataSize = 10;
    spaceForData = lowpanBuffer.getBuffer(dataSize);

    uint8_t dummyinfo[dataSize];
    for(uint8_t i = 0; i < dataSize; i++){
        dummyinfo[i] = i;
    }

    spaceForData->copyToBuffer(dummyinfo, dataSize, 0);

    cometos::Airframe first_frame;
    spaceForData->populateFrame(first_frame, 4, 5);
    uint8_t a;
    uint8_t i;

    for(i = 4; i < 9; i++){
        first_frame >> a;
        EXPECT_EQ(a, i);
    }

    cometos::Airframe second_frame;
    spaceForData->populateFrame(second_frame, 0, 15);
    EXPECT_EQ(second_frame.getLength(), 10);
    for(uint8_t i = 0; i < 10; i++){
        second_frame >> a;
        EXPECT_EQ(i, a);
    }

/*    cometos::Airframe third_frame;
    spaceForData->populateFrame(third_frame, 10, 15);
    EXPECT_EQ(0, third_frame.getLength());*/

}
}

