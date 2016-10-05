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

#include "RBTree.h"

#include <algorithm>
#include <cstdlib>

#define TEST_MAX_RBTREE 100

namespace {

class RBTreeUnittest : public ::testing::Test {
public:
    typedef dsme::RBTree<int, int>::iterator iterator;

    RBTreeUnittest() {
    }

    void SetUp() {
    }

    void TearDown() {
    }
protected:
    dsme::RBTree<int, int> tree;

    void printTree(dsme::RBTree<int, int> t) {
        printNode(t.getRoot(), 0);
    }

    void printNode(dsme::RBNode<int, int>* current, int indent) {
        if (current != nullptr) {
            printNode(current->rightChild, indent + 4);
            if (indent > 0)
                std::cout << std::setw(indent) << " ";
            std::cout << current->key << "[";
            if (current->parent) {
                std::cout << current->parent->key;
            } else {
                std::cout << "null";
            }
            std::cout << "]";
            std::cout << "(" << std::string((current->color == dsme::RED) ? "R" : "B") << ")" << std::endl;
            printNode(current->leftChild, indent + 4);
        }
    }

    void fillWithIteratedPreOrder(dsme::RBNode<int, int>* current, std::vector<int> &linear) {
        if (current == nullptr) {
            return;
        }
        linear.push_back(current->content);
        fillWithIteratedPreOrder(current->leftChild, linear);
        fillWithIteratedPreOrder(current->rightChild, linear);
    }

    void fillWithIteratedPostOrder(dsme::RBNode<int, int>* current, std::vector<int> &linear) {
        if (current == nullptr) {
            return;
        }
        fillWithIteratedPostOrder(current->leftChild, linear);
        fillWithIteratedPostOrder(current->rightChild, linear);
        linear.push_back(current->content);
    }

};

/**
 * Test corner cases for an empty tree:
 *      -In an empty tree the first and last iterator point to the same.
 *      -After inserting and removing the first and last iterator point to the same.
 *      -After inserting one element begin() and end() do not point to the same.
 *
 */
TEST_F(RBTreeUnittest, emptyTree) {

    EXPECT_EQ(tree.begin(), tree.end());

    tree.insert(1, 1);
    iterator iter = tree.find(1);
    EXPECT_EQ(tree.begin(), iter);
    EXPECT_EQ(tree.end(), ++iter);

    iter = tree.find(1);
    tree.remove(iter);

    EXPECT_EQ(tree.begin(), tree.end());

}

/**
 * Inserting TEST_MAX_RBTREE elements:
 *      - every element will be found
 *      - iteration will touch every element
 *      - operator++ for the end()-iterator results in an end()-iterator
 */
TEST_F(RBTreeUnittest, IterateOverTree) {
    std::set<int> compare;

    for (int i = 0; i < TEST_MAX_RBTREE; i++) {
        tree.insert(i, i);
        compare.insert(i);
    }

    for (int i = 0; i < TEST_MAX_RBTREE; i++) {
        iterator iter = tree.find(i);
        EXPECT_EQ(i, *iter);
    }

    iterator iter = tree.begin();
    for (int i = 0; i < TEST_MAX_RBTREE; i++) {
        EXPECT_TRUE(compare.count(*iter) == 1);
        compare.erase(*iter);

        EXPECT_NE(tree.end(), iter);
        ++iter;
    }

    EXPECT_EQ(tree.end(), iter);
    EXPECT_TRUE(compare.empty());

    iter++;
    EXPECT_EQ(tree.end(), iter);
}

/**
 * Inserting TEST_MAX_RBTREE elements:
 *      - iteration is done in the correct order
 */
TEST_F(RBTreeUnittest, CorrectIterationOrder) {
    std::vector<int> actual, expected;

    for (int i = 0; i < TEST_MAX_RBTREE; i++) {
        tree.insert(i, i);
    }
    for (iterator it = tree.begin(); it != tree.end(); ++it) {
        actual.push_back(*it);
    }
#ifdef RBTREE_ITERATOR_POSTORDER
    std::cout << "[          ] POSTORDER" << std::endl;
    fillWithIteratedPostOrder(tree.getRoot(), expected);
#else
    std::cout << "[          ] PREORDER" << std::endl;
    fillWithIteratedPreOrder(tree.getRoot(), expected);
#endif

    EXPECT_EQ(expected, actual);
}

/**
 * Inserting TEST_MAX_RBTREE elements in fixed order:
 *      -every element can be deleted
 */
TEST_F(RBTreeUnittest, deleteFromTree) {
    int i;
    std::set<int> compare;

    for (i = 0; i < TEST_MAX_RBTREE; i++) {
        tree.insert(i, i);
        compare.insert(i);

        EXPECT_EQ(i + 1, tree.size());
    }

    for (i = 0; i < TEST_MAX_RBTREE; i++) {
        iterator iter = tree.begin();
        int numberDeleted = *iter;

        EXPECT_TRUE(compare.count(numberDeleted) == 1);
        compare.erase(numberDeleted);

        tree.remove(iter);
        EXPECT_EQ(tree.end(), iter);
        EXPECT_EQ(tree.end(), tree.find(numberDeleted));

        for (int t : compare) {
            EXPECT_NE(tree.end(), tree.find(t));
        }
    }

    EXPECT_TRUE(compare.empty());
    EXPECT_EQ(0, tree.size());
}

/**
 * Inserting TEST_MAX_RBTREE elements randomly:
 *      -every element can be deleted
 */
TEST_F(RBTreeUnittest, deleteFromTreeRandom) {
    std::set<int> compare;
    srand(1);

    /*************
     * Insertion *
     *************/

    {
        for (int i = 0; i < TEST_MAX_RBTREE; i++) {
            compare.insert(i);
        }
        std::set<int> bucket(compare);

        while (!bucket.empty()) {
            std::set<int>::iterator it = bucket.begin();
            advance(it, rand() % bucket.size());
            tree.insert(*it, *it);
            bucket.erase(*it);
            EXPECT_EQ(TEST_MAX_RBTREE - bucket.size(), tree.size());
        }
        EXPECT_EQ(compare.size(), tree.size());
    }

    /*************
     * Deletion  *
     *************/
    {
        std::set<int> bucket(compare);
        while (!bucket.empty()) {
            std::set<int>::iterator it = bucket.begin();
            advance(it, rand() % bucket.size());
            int key = *it;
            iterator iter = tree.find(key);

            compare.erase(*iter);

            tree.remove(iter);
            bucket.erase(key);

            EXPECT_EQ(tree.end(), iter);
            EXPECT_EQ(tree.end(), tree.find(key));
            EXPECT_EQ(bucket.size(), tree.size());

            for (int otherKey : bucket) {
                EXPECT_NE(tree.end(), tree.find(otherKey));
            }
        }
        EXPECT_TRUE(bucket.empty());
        EXPECT_EQ(0, tree.size());
    }
}

}
