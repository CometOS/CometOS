/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * introduced in the IEEE 802.15.4e-2012 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
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

#include "BeaconBitmap.h"

#define BYTE_SIZE 50
namespace {
class BeaconBitmapUnittest: public ::testing::Test {
public:
    BeaconBitmapUnittest() {
        EXPECT_EQ(true, bitmap.setSDBitmapLengthBytes(BYTE_SIZE,false));
    }

    void SetUp() {
    }

    void TearDown() {
    }

protected:
    dsme::BeaconBitmap bitmap;
};

/*
 * handles the corner cases for the getNextAllocated() function
 *  -no slot is allocated -> -1
 *  -ever slot is allocated -> 0
 *  -one slot is allocated -> that slot
 *  -more than one slot allocated -> smallest one
 *  -after clearing one slot -> smallest slot or -1 if that was the only one
 *  -not starting at position 0 -> smallest one which is bigger than position
 */
TEST_F(BeaconBitmapUnittest, getNextAllocated) {
    bitmap.set(3, true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 3);
    bitmap.set(3, false);
    EXPECT_EQ(bitmap.getNextAllocated(0), -1);
    bitmap.set(5, true);
    bitmap.set(1, true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 1);
    bitmap.set(2, true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 1);
    bitmap.set(1, false);
    EXPECT_EQ(bitmap.getNextAllocated(0), 2);
    EXPECT_EQ(bitmap.getNextAllocated(3), 5);
    bitmap.fill(true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 0);
    bitmap.fill(false);
    EXPECT_EQ(bitmap.getNextAllocated(0), -1);
}

/*
 * handles the corner cases for the getNextFreeSlot() function
 *  -only the first slot is allocated -> 1
 *  -the first and one further (but not the next) is allocated -> 1
 *  -one slot but not the first is allocated -> 0
 *  -every slot is allocated -> -1
 *  -no slot is allocated -> 0
 */
TEST_F(BeaconBitmapUnittest, getNextFreeSlot) {
    bitmap.set(0, true);
    EXPECT_EQ(bitmap.getNextFreeSlot(), 1);
    bitmap.set(3, true);
    EXPECT_EQ(bitmap.getNextFreeSlot(), 1);
    bitmap.set(0, false);
    EXPECT_EQ(bitmap.getNextFreeSlot(), 0);
    bitmap.fill(true);
    EXPECT_EQ(bitmap.getNextFreeSlot(), -1);
    bitmap.fill(false);
    EXPECT_EQ(bitmap.getNextFreeSlot(), 0);
}

/*
 * handles the corner cases for the getRandomFreeSlot() function
 *  -no slot allocated -> not -1
 *  -all slots allocated -> -1
 *  -all slots except one allocated -> that one
 *  -only one allocated -> not that one and not -1
 */
TEST_F(BeaconBitmapUnittest, getRandomFreeSlot) {
    bitmap.set(3, true);
    EXPECT_NE(bitmap.getRandomFreeSlot(rand()), 3);
    EXPECT_NE(bitmap.getRandomFreeSlot(rand()), -1);

    bitmap.fill(true);
    EXPECT_EQ(bitmap.getRandomFreeSlot(rand()), -1);

    bitmap.set(5, false);
    EXPECT_EQ(bitmap.getRandomFreeSlot(rand()), 5);

    bitmap.fill(false);
    EXPECT_NE(bitmap.getRandomFreeSlot(rand()), -1);
}

/*
 * handles the corner cases for the getAllocatedCount() function
 *  -lowest value : bitmap is empty -> 0
 *  -highest value: bitmap is completely full -> 8*SIZE
 *  -adding one value with set -> 1
 *
 */
TEST_F(BeaconBitmapUnittest, getAllocatedCount) {
    bitmap.set(3, true);
    EXPECT_EQ(bitmap.getAllocatedCount(), 1);
    bitmap.fill(true);
    EXPECT_EQ(bitmap.getAllocatedCount(), 8* BYTE_SIZE);
    bitmap.fill(false);
    EXPECT_EQ(bitmap.getAllocatedCount(), 0);
}

/*
 * handles the corner cases for the set function
 * -same bit is set twice       -> the right bit is set and there is only that bit set
 * -same bit is cleared twice   -> right bit is cleared and only that one
 *
 */
TEST_F(BeaconBitmapUnittest, set) {
    bitmap.set(5, true);

    bitmap.set(3, true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 3);
    EXPECT_EQ(bitmap.getAllocatedCount(), 2);
    bitmap.set(3, true);
    EXPECT_EQ(bitmap.getNextAllocated(0), 3);
    EXPECT_EQ(bitmap.getAllocatedCount(), 2);

    bitmap.set(3, false);
    EXPECT_EQ(bitmap.getAllocatedCount(), 1);
    EXPECT_EQ(bitmap.getNextAllocated(0), 5);
    bitmap.set(3, false);
    EXPECT_EQ(bitmap.getAllocatedCount(), 1);
    EXPECT_EQ(bitmap.getNextAllocated(0), 5);
}

/*
 * handles the corner cases for the get function
 * -before setting a bit -> 0
 * -after setting a bit  -> 1 when getting that bit else 0
 *
 */
TEST_F(BeaconBitmapUnittest, get) {
    EXPECT_EQ(bitmap.get(0), 0);
    bitmap.set(0, true);
    EXPECT_EQ(bitmap.get(0), 1);
    EXPECT_EQ(bitmap.get(1), 0);

    bitmap.fill(false);
}

/*
    bitmap.fill(false);
 * handles the corner cases for the copyBitsFrom function
 * -same size -> same bits
 * -smaller   -> same bits up to smaller size
 *
 */
TEST_F(BeaconBitmapUnittest, copyBitsFrom) {
    dsme::BeaconBitmap other;
    other.setSDBitmapLengthBytes(BYTE_SIZE, false);
    bool r;
    for (int i = 0; i < BYTE_SIZE * 8; i++) {
        r = rand() % 2;
        other.set(i, r);
    }
    bitmap.copyBitsFrom(other);

    for (int i = 0; i < BYTE_SIZE * 8; i++) {
        EXPECT_EQ(bitmap.get(i), other.get(i));
    }

    dsme::BeaconBitmap other2;
    other2.setSDBitmapLengthBytes(BYTE_SIZE / 2, false);
    for (int i = 0; i < BYTE_SIZE * 8; i++) {
        r = rand() % 2;
        bitmap.set(i, r);
    }
    other2.copyBitsFrom(bitmap);
    for (int i = 0; i < BYTE_SIZE * 4; i++) {
        EXPECT_EQ(bitmap.get(i), other2.get(i));
    }
}

/*
 * handles the corner cases for the orWith function
 * same size required
 *  -both 0 -> 0
 *  -one 0 and one 1 -> 1
 *  -both 1 ->1
 *
 */
TEST_F(BeaconBitmapUnittest, orWith) {
    dsme::BeaconBitmap other;
    other.setSDBitmapLengthBytes(BYTE_SIZE, false);
    bitmap.set(0, true);
    other.set(0, true);
    bitmap.orWith(other);
    EXPECT_EQ(bitmap.get(0), true);

    bitmap.set(0, false);
    other.set(0, false);
    bitmap.orWith(other);
    EXPECT_EQ(bitmap.get(0), false);

    bitmap.set(0, true);
    other.set(0, false);
    bitmap.orWith(other);
    EXPECT_EQ(bitmap.get(0), true);

    bitmap.set(0, false);
    other.set(0, true);
    bitmap.orWith(other);
    EXPECT_EQ(bitmap.get(0), true);
}

}
