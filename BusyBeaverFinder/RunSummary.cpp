//
//  RunSummary.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "RunSummary.h"

#include <iostream>
#include <vector>

#include "ProgramBlock.h"
#include "InterpretedProgram.h"
#include "Utils.h"

void RunSummaryBase::reset() {
    _runBlocks.clear();

    _processed = 0;
    _pending = 0;
    _loop = -1;

    _sequenceBlocks.clear();
    _sequenceBlocks.emplace_back(0);

    _rotationEqualityCache.clear();
}

RunBlockSequenceNode* RunSummaryBase::getChildNode(
    RunBlockSequenceNode* parent, RunUnitId targetId
) {
    RunBlockSequenceNode* node = parent;

    if (!node->_childIndex) {
        // This node does not yet have any children. Add the first.
        node->_childIndex = (int)_sequenceBlocks.size();
        _sequenceBlocks.emplace_back(targetId);
        return &_sequenceBlocks.back();
    }

    node = &_sequenceBlocks[node->_childIndex];
    if (node->getRunUnitId() == targetId) {
        // Found it!
        return node;
    }

    // Check the siblings
    while (node->_siblingIndex) {
        node = &_sequenceBlocks[node->_siblingIndex];
        if (node->getRunUnitId() == targetId) {
            // Found it!
            return node;
        }
    }

    node->_siblingIndex = (int)_sequenceBlocks.size();
    _sequenceBlocks.emplace_back(targetId);
    return &_sequenceBlocks.back();
}

void RunSummaryBase::createRunBlock(int start, int end, int loopPeriod) {
    RunBlockSequenceNode* sequenceNodeP = &_sequenceBlocks[0];

    for (int i = start; i < end; ++i) {
        sequenceNodeP = getChildNode(sequenceNodeP, getRunUnitIdAt(i));
    }

    int sequenceId = (int)(sequenceNodeP - &_sequenceBlocks[0]);

    _runBlocks.emplace_back(start, sequenceId, loopPeriod);
}

int RunSummaryBase::getRunBlockLength(int startIndex, int endIndex) const {
    bool isLast = endIndex == getNumRunBlocks();
    int runUnitStartIndex = _runBlocks[startIndex].getStartIndex();

    if (isLast) {
        if (_pending > 0) {
            // Last run block is completed. There are still some unassigned run units though
            return _pending - runUnitStartIndex;
        } else {
            return getNumRunUnits() - runUnitStartIndex;
        }
    } else {
        return _runBlocks[endIndex].getStartIndex() - runUnitStartIndex;
    }
}

int RunSummaryBase::getLoopIteration() const {
    assert(_loop >= 0);

    auto loopBlock = _runBlocks.back();
    int startIndex = loopBlock.getStartIndex();
    int loopLength = getNumRunUnits() - startIndex;

    return loopLength / loopBlock.getLoopPeriod();
}

bool RunSummaryBase::isAtEndOfLoop() const {
    assert(_loop >= 0);

    auto loopBlock = _runBlocks.back();
    int startIndex = loopBlock.getStartIndex();
    int loopLength = getNumRunUnits() - startIndex;

    return loopLength % loopBlock.getLoopPeriod() == 0;
}

// Implements Booth's algorithm
// See: https://en.wikipedia.org/wiki/Lexicographically_minimal_string_rotation#Booth's_Algorithm
int RunSummaryBase::calculateCanonicalLoopIndex(int startIndex, int len) const {
    // Initialize failure function
    std::vector<int> f(len, -1);

    int k = 0; // Least rotation of sequence found so far

    for (int j = 1; j < len; ++j) {
        int sj = getRunUnitIdAt(startIndex + j);
        int i = f[j - k - 1];
        while (i != -1 && sj != getRunUnitIdAt(startIndex + k + i + 1)) {
            if (sj < getRunUnitIdAt(startIndex + k + i + 1)) {
                k = j - i - 1;
            }
            i = f[i];
        }
        if (sj != getRunUnitIdAt(startIndex + k + i + 1)) {
            assert(i == -1);
            if (sj < getRunUnitIdAt(startIndex + k)) { // k + i + 1 == k
                k = j;
            }
            f[j - k] = -1;
        } else {
            f[j - k] = i + 1;
        }
    }

    return startIndex + k;
}

bool RunSummaryBase::determineRotationEquivalence(int index1, int index2, int len,
                                                  int &offset) const {
    int ci1 = calculateCanonicalLoopIndex(index1, len);
    int ci2 = calculateCanonicalLoopIndex(index2, len);

    for (int i = len; --i >= 0; ) {
        if (getRunUnitIdAt(ci1 + i) != getRunUnitIdAt(ci2 + i)) {
            return false;
        }
    }

    int relIndex1 = ci1 - index1;
    int relIndex2 = ci2 - index2;
    offset = (relIndex1 + len - relIndex2) % len;

    return true;
}


int RunSummaryBase::areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                                      int &indexOffset) const {
    int index1 = block1->getSequenceId();
    int index2 = block2->getSequenceId();
    if (index1 == index2) {
        // Blocks are equal, even without rotating
        return true;
    }

    // Require that run blocks are loops. This avoids modular arithmetic when executing Booth's
    // algorithm.
    assert(block1->isLoop());
    assert(block2->isLoop());

    int len = block1->getLoopPeriod();
    if (len != block2->getLoopPeriod()) {
        return false;
    }

    int minIndex = std::min(index1, index2);
    int maxIndex = std::max(index1, index2);

    auto map = _rotationEqualityCache;
    auto key = std::make_pair(minIndex, maxIndex);
    auto cachedResult = map.find(key);
    if (cachedResult != map.end()) {
        // Return previously calculated result
        indexOffset = cachedResult->second.second;
        return cachedResult->second.first;
    }

    bool areEqual = determineRotationEquivalence(block1->getStartIndex(),
                                                block2->getStartIndex(),
                                                len, indexOffset);
    // Cache result
    map[key] = std::make_pair(areEqual, indexOffset);

    return areEqual;
}

void RunSummaryBase::dumpRunBlockSequenceNode(int nodeIndex, int level) const {
    for (int i = 0; i < level; i++) {
        std::cout << "  ";
    }

    auto &node = _sequenceBlocks[nodeIndex];
    std::cout << nodeIndex << " (" << node.getRunUnitId() << ")" << std::endl;

    // Dump children
    if (node._childIndex) {
        dumpRunBlockSequenceNode(node._childIndex, level + 1);
    }

    // Dump siblings
    if (node._siblingIndex) {
        dumpRunBlockSequenceNode(node._siblingIndex, level);
    }
}

void RunSummaryBase::dumpSequenceTree() const {
    dumpRunBlockSequenceNode(0, 0);
}

void RunSummaryBase::dumpCondensed() const {
    for (int i = 0; i < getNumRunBlocks(); i++) {
        const RunBlock* runBlock = runBlockAt(i);
        int sequenceIndex = runBlock->getSequenceId();

        if (i > 0) {
            std::cout << " ";
        }

        std::cout << "#" << sequenceIndex;

        if (runBlock->isLoop()) {
            int len = getRunBlockLength(i);
            int period = runBlock->getLoopPeriod();

            std::cout << "*" << len / period << "." << (len % period);
        }
    }
    std::cout << std::endl;

    int numUnique = 0;
    int helperArray[64];

    std::cout << "with:" << std::endl;
    for (int i = 0; i < getNumRunBlocks(); i++) {
        const RunBlock* runBlock = runBlockAt(i);
        int sequenceIndex = runBlock->getSequenceId();

        int j = 0;
        while (j < numUnique && helperArray[j] != sequenceIndex) {
            j++;
        }

        if (j == numUnique) {
            // First encounter of this block. So dump it
            std::cout << sequenceIndex << " =";

            int len = getRunBlockLength(i);
            if (runBlock->isLoop()) {
                len = runBlock->getLoopPeriod();
            }

            int k = runBlock->getStartIndex();
            int end = k + len;
            while (k < end) {
                std::cout << " " << getRunUnitIdAt(k++);
            }
            std::cout << std::endl;

            helperArray[numUnique++] = sequenceIndex;
            if (numUnique == 64) {
                return;
            }
        }
    }
}

void RunSummaryBase::dump() const {
    int runUnitIndex = 0;
    int numRunUnits = getNumRunUnits();
    auto runBlock = _runBlocks.cbegin();
    int numPendingBlocks = 0;
    bool isLoop = false;

    while (runUnitIndex < numRunUnits) {
        if (--numPendingBlocks == 0) {
            std::cout << (isLoop ? "] " : ") ");
        }
        if (
            numPendingBlocks <= 0 &&
            runBlock != _runBlocks.cend() &&
            (*runBlock).getStartIndex() == runUnitIndex
        ) {
            isLoop = (*runBlock).isLoop();
            std::cout << "#" << (*runBlock).getSequenceId() << (isLoop ? "[" : "(");
            numPendingBlocks = getRunBlockLength(&*runBlock);

            runBlock++;
        } else {
            std::cout << " ";
        }
        std::cout << getRunUnitIdAt(runUnitIndex++);
    }
    std::cout << std::endl;
}

int RunSummary::getDpDeltaOfProgramBlockSequence(int start, int end) const {
    int dpDelta = 0;

    for (auto programBlock : makeRange(_runHistory, start, end)) {
        if (!programBlock->isDelta()) {
            dpDelta += programBlock->getInstructionAmount();
        }
    }

    return dpDelta;
}

int RunSummary::getDpDelta(int firstRunBlock, int lastRunBlock) const {
    int dpDelta = 0;

    for (auto runBlock : makeRange(_runBlocks, firstRunBlock, lastRunBlock)) {
        int runBlockLen = getRunBlockLength(&runBlock);
        int start = runBlock.getStartIndex();

        if (runBlock.isLoop()) {
            int loopPeriod = runBlock.getLoopPeriod();
            int dpDeltaPerIteration = getDpDeltaOfProgramBlockSequence(start, start + loopPeriod);
            int numIterations = runBlockLen / loopPeriod;
            int lenFullSpins = loopPeriod * numIterations;

            dpDelta += numIterations * dpDeltaPerIteration;
            dpDelta += getDpDeltaOfProgramBlockSequence(start + lenFullSpins, start + runBlockLen);
        } else {
            dpDelta += getDpDeltaOfProgramBlockSequence(start, start + runBlockLen);
        }
    }

    return dpDelta;
}
