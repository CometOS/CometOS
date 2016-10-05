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
#include "DSMEBitVector.h"
#include "neighbors/NeighborQueue.h"
#include "neighbors/Neighbor.h"

namespace {

constexpr uint8_t MAX_SIZE = 30;

class BitVectorUnittest : public ::testing::Test {
public:

    BitVectorUnittest() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
protected:
    void printVector(const dsme::BitVector<MAX_SIZE>& bv, std::string name = "") {
        if (name != "") {
            std::cout << name << ": ";
        }
        for (int i = 0; i < bv.length(); ++i) {
            std::cout << (bv.get(i) ? "1" : "0");
            if (i != 0 && (i + 1) % 8 == 0) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
        return;
    }
};

TEST_F(BitVectorUnittest, complementNoOffset) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, true);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 0;

    vector.setOperationComplement(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, false);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, complementWithOffset) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, true);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 5;

    vector.setOperationComplement(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, false);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, complementWithOffsetDiv8) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, true);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 8;

    vector.setOperationComplement(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, false);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, joinNoOffset) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, false);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 0;

    vector.setOperationJoin(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, true);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, joinWithOffset) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, false);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 5;

    vector.setOperationJoin(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, true);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, joinWithOffsetDiv8) {
    dsme::BitVector<MAX_SIZE> vector;
    vector.setLength(20, false);
    dsme::BitVector<MAX_SIZE> copy(vector);

    dsme::BitVector<MAX_SIZE> mask;
    mask.setLength(10, true);

    int offset = 8;

    vector.setOperationJoin(mask, offset);

    EXPECT_NE(copy, vector);

    for (int i = offset; i < offset + mask.length(); ++i) {
        copy.set(i, true);
    }

    EXPECT_EQ(copy, vector);
}

TEST_F(BitVectorUnittest, isZero) {
    int w = 20;

    dsme::BitVector<30> vector;
    vector.setLength(w, true);
    EXPECT_FALSE(vector.isZero());
    for (int i = 0; i < w - 1; ++i) {
        vector.set(i, false);
        EXPECT_FALSE(vector.isZero());
    }
    vector.set(w - 1, false);

    EXPECT_TRUE(vector.isZero());
}

TEST_F(BitVectorUnittest, count) {
    int w = 20;

    dsme::BitVector<30> vector;
    vector.setLength(w, false);
    EXPECT_EQ(0, vector.count(true));
    EXPECT_EQ(w, vector.count(false));
    for (int i = 0; i < w; ++i) {
        vector.set(i, true);
        EXPECT_EQ(i + 1, vector.count(true));
        EXPECT_EQ(w - i - 1, vector.count(false));
    }

    EXPECT_EQ(0, vector.count(false));
    EXPECT_EQ(w, vector.count(true));
}

TEST_F(BitVectorUnittest, iterator) {
    dsme::BitVector<30> vector;
    vector.setLength(30, false);
    dsme::BitVector<30>::iterator it = vector.beginSetBits();
    EXPECT_EQ(vector.endSetBits(), it);

    it++;
    EXPECT_EQ(vector.endSetBits(), it);

    int pos = 5;
    vector.set(pos, true);
    it = vector.beginSetBits();
    EXPECT_EQ(pos, *it);

    ++it;
    EXPECT_EQ(vector.endSetBits(), it);
    EXPECT_EQ(*it, vector.length());

    it = vector.beginUnsetBits();
    EXPECT_EQ(0,*it);
    for(int i = 0; i < pos; i++) {
        EXPECT_EQ(i,*it++);
    }
    EXPECT_EQ(pos+1,*it);

    EXPECT_NE(vector.endSetBits(), vector.endUnsetBits());

}

}
