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

#include "neighbors/NeighborQueue.h"
#include "neighbors/Neighbor.h"

#include "platform/dsme_settings.h"
#include "DSMEMessage.h"

#define MAX_NEIGHBORS 20
constexpr uint16_t NQ_TEST_MAX_QUEUESIZE = dsme::TOTAL_GTS_QUEUE_SIZE;

namespace {
class NeighborQueueUnittest: public ::testing::Test {
public:
    typedef dsme::NeighborQueue<MAX_NEIGHBORS>::iterator iterator;

    NeighborQueueUnittest() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
protected:
    dsme::NeighborQueue<MAX_NEIGHBORS> queue;
    dsme::IEEE802154MacAddress a = 1, b = 2, c = 3;
};

/*
 * handles the corner cases for the addNeighbor() function
 * -after adding an element the number of neighbors is increased by 1
 * -adding the same neighbor twice only adds it once
 *
 * -addNeighbor to full list ->nothing happens
 */
TEST_F(NeighborQueueUnittest, addNeighbor) {
    dsme::Neighbor n(a), m(b);

    queue.addNeighbor(n);
    EXPECT_EQ(1, (int )queue.getNumNeighbors());

    queue.addNeighbor(n);
    EXPECT_EQ(1, (int )queue.getNumNeighbors());

    queue.addNeighbor(m);
    EXPECT_EQ(2, (int )queue.getNumNeighbors());
}

/*
 * handles the corner cases for the eraseNeighbor() function
 * -after erasing an element the number of neighbors is decreased by 1
 * -erasing the same neighbor twice only erase it once and decrease the number only by one
 *
 */
TEST_F(NeighborQueueUnittest, eraseNeighbor) {
    dsme::Neighbor n(a), m(b);

    queue.addNeighbor(n);
    queue.addNeighbor(m);
    EXPECT_EQ(2, (int )queue.getNumNeighbors());

    iterator it = queue.findByAddress(n.address);
    queue.eraseNeighbor(it);
    EXPECT_EQ(1, (int )queue.getNumNeighbors());
    EXPECT_EQ(queue.end(), it);

    queue.eraseNeighbor(it);
    EXPECT_EQ(1, (int )queue.getNumNeighbors());

    it = queue.findByAddress(m.address);
    queue.eraseNeighbor(it);
    EXPECT_EQ(0, (int )queue.getNumNeighbors());
}

/*
 * handles the corner cases for the getNumNeighbor() function
 * - adding increments the number of neighbors
 * - erasing decrements number of neighbors
 * - 0 if no neighbor in the list
 *
 * -number of neighbor is not greater than the NeighborList maximum
 *
 */
TEST_F(NeighborQueueUnittest, getNumNeighbor) {
    dsme::Neighbor n(a), m(b);

    EXPECT_EQ(queue.getNumNeighbors(), 0);

    queue.addNeighbor(n);
    EXPECT_EQ(queue.getNumNeighbors(), 1);

    queue.addNeighbor(m);
    EXPECT_EQ(queue.getNumNeighbors(), 2);

    iterator begin_it = queue.begin();
    queue.eraseNeighbor(begin_it);
    EXPECT_EQ(queue.getNumNeighbors(), 1);
}

/*
 * handles the corner cases for the findByAddress() function
 * - address exist -> expect that searching for the element with that address, the elements address is the set one
 * - search for a not existing address -> expect the end() iterator
 * - add 20 addresses, expect to find each of them
 *
 */
TEST_F(NeighborQueueUnittest, findByAddress) {
    dsme::Neighbor n(a);

    queue.addNeighbor(n);
    iterator it = queue.findByAddress(a);
    EXPECT_EQ((*it).address, a);
    EXPECT_EQ(queue.end(), queue.findByAddress(b));

    queue.eraseNeighbor(it);

    dsme::IEEE802154MacAddress addresses[MAX_NEIGHBORS + 1];
    for (int i = 0; i < MAX_NEIGHBORS + 1; i++) {
        addresses[i] = dsme::IEEE802154MacAddress(100 + i);
        dsme::Neighbor nx(addresses[i]);
        queue.addNeighbor(nx);
    }

    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        it = queue.findByAddress(addresses[i]);
        dsme::Neighbor &res = (*it);
        EXPECT_EQ(100 + i, res.address.a4());
    }
    it = queue.findByAddress(addresses[MAX_NEIGHBORS]);
    EXPECT_EQ(queue.end(),it);
}

/*
 * handles the corner cases for the getPacketsInQueue function
 * - no Packet for that neighbor in Packetqueue -> number is 0
 * - one packet in Queue -> 1 for that queue
 * - full queue -> NQ_TEST_MAX_QUEUESIZE for that queue
 * - full queue + 1 -> NQ_TEST_MAX_QUEUESIZE
 * - two queues have together maximal NQ_TEST_MAX_QUEUESIZE elements
 *
 */
TEST_F(NeighborQueueUnittest, getPacketsInQueue) {
    dsme::Neighbor n(a), m(b);
    queue.addNeighbor(n);
    queue.addNeighbor(m);
    iterator it_n = queue.findByAddress(n.address);
    iterator it_m = queue.findByAddress(m.address);
    EXPECT_EQ(0, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(0, queue.getPacketsInQueue(it_m));

    dsme::DSMEMessage msg;
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE; i++) {
        queue.pushBack(it_n, &msg);
        EXPECT_EQ(i, queue.getPacketsInQueue(it_n));
        EXPECT_EQ(0, queue.getPacketsInQueue(it_m));
    }
    queue.pushBack(it_n, &msg);
    EXPECT_EQ(NQ_TEST_MAX_QUEUESIZE, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(0, queue.getPacketsInQueue(it_m));

    queue.flushQueues(false);

    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE; i++) {
        queue.pushBack(it_n, &msg);
        queue.pushBack(it_m, &msg);
        EXPECT_GE(NQ_TEST_MAX_QUEUESIZE,
                queue.getPacketsInQueue(it_n) + queue.getPacketsInQueue(it_m));

    }

}

/*
 * handles the corner cases for the isQueueEmpty function
 * - no element in queue -> true
 * - one to full queue + 1 -> false
 * - queue flush -> true
 * - two queues -> filling one does not effect the other
 *
 */
TEST_F(NeighborQueueUnittest, isQueueEmpty) {
    dsme::Neighbor n(a), m(b);
    queue.addNeighbor(n);
    iterator it = queue.findByAddress(n.address);
    queue.addNeighbor(m);
    iterator it2 = queue.findByAddress(m.address);

    EXPECT_TRUE(queue.isQueueEmpty(it));
    EXPECT_TRUE(queue.isQueueEmpty(it2));
    EXPECT_EQ(0, queue.getPacketsInQueue(it));

    dsme::DSMEMessage msg;
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE + 1; i++) {
        queue.pushBack(it, &msg);
        EXPECT_FALSE(queue.isQueueEmpty(it));
        EXPECT_TRUE(queue.isQueueEmpty(it2));
    }
    queue.flushQueues(false);

    EXPECT_TRUE(queue.isQueueEmpty(it));
    EXPECT_TRUE(queue.isQueueEmpty(it2));

    queue.pushBack(it2, &msg);
    EXPECT_TRUE(queue.isQueueEmpty(it));
    EXPECT_FALSE(queue.isQueueEmpty(it2));
    queue.pushBack(it, &msg);
    EXPECT_FALSE(queue.isQueueEmpty(it));
    EXPECT_FALSE(queue.isQueueEmpty(it2));

}

/*
 * handles the corner cases for the flushQueues function
 * - flush an empty queue
 * - flush a full queue and keep front element
 * - flush a full queue and do not keep an element
 * - flush a full queue + 1
 * - flush a queue with just one element
 * - flush a queue and keep one element and then flush it again without keeping the element
 * - flush for two queues
 *
 */
TEST_F(NeighborQueueUnittest, flushQueues) {
    dsme::Neighbor n(a), m(b);
    queue.addNeighbor(n);
    iterator it = queue.findByAddress(n.address);

    //empty queue
    EXPECT_TRUE(queue.isQueueEmpty(it));
    queue.flushQueues(false);
    EXPECT_TRUE(queue.isQueueEmpty(it));
    queue.flushQueues(true);
    EXPECT_TRUE(queue.isQueueEmpty(it));

    dsme::DSMEMessage msg;
    //full queue, not keep first element
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE; i++) {
        queue.pushBack(it, &msg);
    }
    queue.flushQueues(false);
    EXPECT_TRUE(queue.isQueueEmpty(it));
    EXPECT_EQ(0, queue.getPacketsInQueue(it));

    //full queue + 1, not keep first element
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE + 1; i++) {
        queue.pushBack(it, &msg);
    }
    queue.flushQueues(false);
    EXPECT_TRUE(queue.isQueueEmpty(it));

    //full queue, keep first element
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE; i++) {
        queue.pushBack(it, &msg);
    }
    queue.flushQueues(true);
    EXPECT_EQ(1, queue.getPacketsInQueue(it));

    //full queue + 1, keep first element
    for (int i = 1; i <= NQ_TEST_MAX_QUEUESIZE + 1; i++) {
        queue.pushBack(it, &msg);
    }

    /*flush a queue and keep one element and then flush it again without keeping the element
     * and other way round */
    queue.flushQueues(true);
    EXPECT_EQ(1, queue.getPacketsInQueue(it));
    queue.flushQueues(false);
    EXPECT_EQ(0, queue.getPacketsInQueue(it));
    queue.flushQueues(true);
    EXPECT_EQ(0, queue.getPacketsInQueue(it));

    /*
     * second neighbor
     */
    queue.addNeighbor(m);
    iterator it2 = queue.findByAddress(m.address);

    queue.pushBack(it2, &msg);
    queue.pushBack(it, &msg);
    EXPECT_EQ(1, queue.getPacketsInQueue(it2));
    EXPECT_EQ(1, queue.getPacketsInQueue(it));
    queue.flushQueues(false);
    EXPECT_EQ(0, queue.getPacketsInQueue(it2));
    EXPECT_EQ(0, queue.getPacketsInQueue(it));
}
/*
 * handles the corner cases for the flushQueue with keep front
 * - flushQueues(true) keeps front element for each queue
 *
 */
TEST_F(NeighborQueueUnittest, flushQueueKeepFront) {
    dsme::Neighbor n(a), m(b);
    std::vector<dsme::DSMEMessage> vec, vec2;
    queue.addNeighbor(n);
    queue.addNeighbor(m);
    iterator it = queue.findByAddress(n.address);
    iterator it2 = queue.findByAddress(m.address);

    for (int i = 0; i < NQ_TEST_MAX_QUEUESIZE; i++) {
        vec.push_back(dsme::DSMEMessage());
        vec2.push_back(dsme::DSMEMessage());
    }
    for (int i = 0; i < NQ_TEST_MAX_QUEUESIZE; i++) {
        queue.pushBack(it, &(vec[i]));
        queue.pushBack(it2, &(vec2[i]));
    }
    queue.flushQueues(true);
    EXPECT_EQ(&(vec[0]), queue.front(it));
    EXPECT_EQ(&(vec[0]), queue.popFront(it));
    EXPECT_EQ(&(vec2[0]), queue.front(it2));
    EXPECT_EQ(&(vec2[0]), queue.popFront(it2));
}

/*
 * handles the corner cases for the front function
 * - popFront yields the same element as front
 * - empty queue -> front returns nullptr
 */
TEST_F(NeighborQueueUnittest, front) {
    dsme::Neighbor n(a);
    queue.addNeighbor(n);
    iterator it = queue.findByAddress(n.address);
    dsme::DSMEMessage msg, msg2, *front;
    //empty queue
    EXPECT_EQ(nullptr, queue.front(it));

    //popFront gives back the same element as front
    queue.pushBack(it, &msg);
    front = queue.front(it);
    EXPECT_EQ(front, queue.popFront(it));

    queue.pushBack(it, &msg);
    queue.pushBack(it, &msg2);
    front = queue.front(it);
    EXPECT_EQ(front, queue.popFront(it));
    front = queue.front(it);
    EXPECT_EQ(front, queue.popFront(it));

}

/*
 * handles the corner cases for the pushBack function and the popFront function
 * - on element pushed -> find that element
 * - one more than the maximal queuesize is pushed, last one should be discarded
 * - elements were popped in same order they were pushed (FIFO)
 * - each neighbor has its own queue
 * - one push and then pop -> packetsInQueue 0->1->0 and afterwards isQueueEmpty is true
 *
 */
TEST_F(NeighborQueueUnittest, pushBackPopFront) {
    dsme::Neighbor n(a), m(b);
    std::vector<dsme::DSMEMessage> vec, vec2;
    dsme::DSMEMessage *msg, *msg2;
    queue.addNeighbor(n);
    iterator it = queue.findByAddress(n.address);

    for (int i = 0; i < NQ_TEST_MAX_QUEUESIZE; i++) {
        vec.push_back(dsme::DSMEMessage());
    }
    for (int i = 0; i < NQ_TEST_MAX_QUEUESIZE + 1; i++) {
        queue.pushBack(it, &(vec[i]));
    }

    for (int i = 0; i < NQ_TEST_MAX_QUEUESIZE; i++) {
        msg = queue.popFront(it);
        EXPECT_EQ(&(vec[i]), msg);
    }
    EXPECT_EQ(nullptr, queue.popFront(it));

    /*
     * second neighbor
     */
    queue.addNeighbor(m);
    iterator it2 = queue.findByAddress(m.address);

    vec.push_back(dsme::DSMEMessage());
    vec2.push_back(dsme::DSMEMessage());
    queue.pushBack(it, &(vec[0]));
    queue.pushBack(it2, &(vec2[0]));
    msg = queue.popFront(it);
    msg2 = queue.popFront(it2);
    EXPECT_EQ(&(vec[0]), msg);
    EXPECT_EQ(&(vec2[0]), msg2);

    queue.pushBack(it, msg);
    EXPECT_EQ(1, queue.getPacketsInQueue(it));
    queue.popFront(it);
    EXPECT_TRUE(queue.isQueueEmpty(it));
    EXPECT_EQ(0, queue.getPacketsInQueue(it));

}

/*
 * Checks for interference as experienced in issue #9, "Bug in Message Queue?"
 * Covers a specific case where a dangling messageBack pointer lead to overwriting other neighbors queues.
 */
TEST_F(NeighborQueueUnittest, popAllCrossInteraction) {
    /*
     * Setup queues
     */
    dsme::Neighbor n(a), m(b);
    std::vector<dsme::DSMEMessage> vec, vec2;

    queue.addNeighbor(n);
    queue.addNeighbor(m);
    iterator it_n = queue.findByAddress(n.address);
    iterator it_m = queue.findByAddress(m.address);

    EXPECT_NE(it_n, queue.end());
    EXPECT_NE(it_m, queue.end());
    EXPECT_NE(it_n, it_m);

    EXPECT_EQ(0, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(0, queue.getPacketsInQueue(it_m));

    /*
     * Make sure, that after the last (only) element from queue "n" is removed, the slot is almost at freeFront.
     * -> First insert element for 'n'.
     * -> Fill whole queue-buffer up (doesn't matter that it is for "m", must be different to "n")
     * -> Take element for "n" out.
     * Prior to the fix this lead to "n" still keeping this freed slot as current massageBack,
     *  upon inserting elements into a different queue 'm' (had do cover this slot because it is freeFront)
     *  and then inserting into queue "n" again, the next pointer of "m"'s element in this slot is being overwritten
     */
    dsme::DSMEMessage *msg0_n = new dsme::DSMEMessage();
    queue.pushBack(it_n, msg0_n);

    for (int i = 0; i < dsme::TOTAL_GTS_QUEUE_SIZE - 1; i++) {
        vec.push_back(dsme::DSMEMessage());
    }
    for (int i = 0; i < dsme::TOTAL_GTS_QUEUE_SIZE - 1; i++) {
        queue.pushBack(it_m, &(vec[i]));
    }

    EXPECT_EQ(1, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(dsme::TOTAL_GTS_QUEUE_SIZE - 1, queue.getPacketsInQueue(it_m));

    dsme::DSMEMessage *msg0_n_out = queue.popFront(it_n);
    EXPECT_EQ(msg0_n, msg0_n_out);

    EXPECT_EQ(0, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(dsme::TOTAL_GTS_QUEUE_SIZE - 1, queue.getPacketsInQueue(it_m));

    for (int i = 0; i < dsme::TOTAL_GTS_QUEUE_SIZE - 1; i++) {
        dsme::DSMEMessage *msg = queue.popFront(it_m);
        EXPECT_EQ(&(vec[i]), msg);
    }

    EXPECT_EQ(0, queue.getPacketsInQueue(it_n));
    EXPECT_EQ(0, queue.getPacketsInQueue(it_m));

    /*
     * Insert elements for "m":
     */
    dsme::DSMEMessage *msg1_m = new dsme::DSMEMessage();
    dsme::DSMEMessage *msg2_m = new dsme::DSMEMessage();

    queue.pushBack(it_m, msg1_m);
    queue.pushBack(it_m, msg2_m);

    /*
     * Pre-fix: Overwrite pointer of one of "m"'s elements:
     */
    dsme::DSMEMessage *msg1_n = new dsme::DSMEMessage();
    queue.pushBack(it_n, msg1_n);

    dsme::DSMEMessage *msg1_m_out = queue.popFront(it_m);
    dsme::DSMEMessage *msg2_m_out = queue.popFront(it_m);

    /*
     * Error would occur here:
     */
    EXPECT_EQ(msg1_m, msg1_m_out);
    EXPECT_EQ(msg2_m, msg2_m_out);
}

}
