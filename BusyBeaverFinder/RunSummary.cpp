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
#include "Utils.h"

void RunBlockSequenceNode::init(ProgramBlockIndex programBlockIndex) {
    _programBlockIndex = programBlockIndex;
    _childIndex = 0;
    _siblingIndex = 0;
}

void RunBlock::init(int startIndex, int sequenceIndex, int loopPeriod) {
    _startIndex = startIndex;
    _sequenceIndex = sequenceIndex;
    _loopPeriod = loopPeriod;
}

void RunSummary::freeDynamicArrays() {
    if (_programBlockHistory != nullptr) {
        delete[] _programBlockHistory;
        _programBlockHistory = nullptr;
    }
}

RunSummary::~RunSummary() {
    freeDynamicArrays();
}

void RunSummary::setCapacity(int capacity, int* helperBuf) {
    freeDynamicArrays();

    _programBlockHistory = new ProgramBlockIndex[capacity];
    _programBlockHistoryMaxP = _programBlockHistory + capacity;

    _helperBuf = helperBuf;

    reset();
}

void RunSummary::reset() {
    _programBlockHistoryP = _programBlockHistory;
    _runBlockHistoryP = _runBlockHistory;

    _programBlockPendingP = _programBlockHistory;
    _loopP = nullptr;

    _sequenceBlock[0].init(0);
    _numSequenceBlocks = 1;

    _rotationEqualityCache.clear();
}

RunBlockSequenceNode* RunSummary::getChildNode(
    RunBlockSequenceNode* parent,
    ProgramBlockIndex targetIndex
) {
    RunBlockSequenceNode* node = parent;

    if (!node->_childIndex) {
        // This node does not yet have any children. Add the first.
        assert(_numSequenceBlocks < maxNumSequenceBlocks);

        node->_childIndex = _numSequenceBlocks;
        node = _sequenceBlock + _numSequenceBlocks++;
        node->init(targetIndex);

        return node;
    }

    node = _sequenceBlock + node->_childIndex;
    if (node->getProgramBlockIndex() == targetIndex) {
        // Found it!
        return node;
    }

    // Check the siblings of the first child
    while (node->_siblingIndex) {
        node = _sequenceBlock + node->_siblingIndex;
        if (node->getProgramBlockIndex() == targetIndex) {
            // Found it!
            return node;
        }
    }

    // There's no sequence start yet for this program block. Create it
    assert(_numSequenceBlocks < maxNumSequenceBlocks);

    node->_siblingIndex = _numSequenceBlocks;
    node = _sequenceBlock + _numSequenceBlocks++;
    node->init(targetIndex);

    return node;
}

int RunSummary::getSequenceIndex(ProgramBlockIndex* startP, ProgramBlockIndex* endP) {
    ProgramBlockIndex* blockP = startP;
    RunBlockSequenceNode* sequenceNodeP = _sequenceBlock; // Start at root

    while (blockP != endP) {
        sequenceNodeP = getChildNode(sequenceNodeP, *blockP++);
    }

    return (int)(sequenceNodeP - _sequenceBlock);
}

void RunSummary::createRunBlock(ProgramBlockIndex* startP, ProgramBlockIndex* endP,
                                int loopPeriod) {
    int sequenceIndex = getSequenceIndex(startP, endP);

    assert(_runBlockHistoryP < _runBlockHistoryMaxP);
    (_runBlockHistoryP++)->init((int)(startP - _programBlockHistory), sequenceIndex, loopPeriod);
}

bool RunSummary::recordProgramBlock(ProgramBlockIndex blockIndex) {
//    std::cout << "recordProgramBlock #" << (int)blockIndex << std::endl;

    assert(_programBlockHistoryP != _programBlockHistoryMaxP);
    *_programBlockHistoryP = blockIndex;

    bool newRunBlocks = false;

    if (_loopP == nullptr) {
        int loopPeriod = findRepeatedSequence(
            _programBlockPendingP,
            _helperBuf,
            (int)(_programBlockHistoryP - _programBlockPendingP + 1)
        );
        if (loopPeriod > 0) {
//            std::cout << "Loop detected (period = " << loopPeriod << ")" << std::endl;
            ProgramBlockIndex* loopStartP = _programBlockHistoryP - loopPeriod * 2 + 1;

            if (loopStartP != _programBlockPendingP) {
                createRunBlock(_programBlockPendingP, loopStartP, 0);
            }
            createRunBlock(loopStartP, _programBlockHistoryP, loopPeriod);

            _loopP = _programBlockHistoryP - loopPeriod + 1;
            _programBlockPendingP = nullptr;
            newRunBlocks = true;
        }
    } else {
        if (*_loopP++ != *_programBlockHistoryP) {
//            std::cout << "Loop exited!" << std::endl;
            // Loop is broken
            _programBlockPendingP = _programBlockHistoryP;
            _loopP = nullptr;
        }
    }

    _programBlockHistoryP++;

    return newRunBlocks;
}

int RunSummary::getRunBlockLength(int startIndex, int endIndex) const {
    bool isLast = endIndex == getNumRunBlocks();
    const RunBlock* runBlockP = _runBlockHistory + startIndex;
    int programBlockStartIndex = runBlockP->getStartIndex();

    if (isLast) {
        if (_programBlockPendingP != nullptr) {
            // Last run is completed. There are still some unallocated program blocks though
            return (int)(_programBlockPendingP - _programBlockHistory) - programBlockStartIndex;
        } else {
            return (int)(_programBlockHistoryP - _programBlockHistory) - programBlockStartIndex;
        }
    } else {
        return (_runBlockHistory + endIndex)->getStartIndex() - programBlockStartIndex;
    }
}

int RunSummary::getLoopIteration() const {
    assert(_loopP != nullptr);

    int startIndex = (_runBlockHistoryP - 1)->getStartIndex();
    int loopLength = (int)(_programBlockHistoryP - _programBlockHistory) - startIndex;

    return loopLength / getLoopPeriod();
}

bool RunSummary::isAtEndOfLoop() const {
    assert(_loopP != nullptr);
    assert(_programBlockPendingP == nullptr);

    int startIndex = (_runBlockHistoryP - 1)->getStartIndex();
    int loopLength = (int)(_programBlockHistoryP - _programBlockHistory) - startIndex;

    return (loopLength % getLoopPeriod() == 0);
}

// Implements Booth's algorithm
// See: https://en.wikipedia.org/wiki/Lexicographically_minimal_string_rotation#Booth's_Algorithm
int RunSummary::calculateCanonicalLoopIndex(int startIndex, int len) const {
    // Initialize failure function
    std::vector<int> f(len, -1);

    int k = 0; // Least rotation of sequence found so far

    for (int j = 1; j < len; ++j) {
        int sj = programBlockIndexAt(startIndex + j);
        int i = f[j - k - 1];
        while (i != -1 && sj != programBlockIndexAt(startIndex + k + i + 1)) {
            if (sj < programBlockIndexAt(startIndex + k + i + 1)) {
                k = j - i - 1;
            }
            i = f[i];
        }
        if (sj != programBlockIndexAt(startIndex + k + i + 1)) {
            assert(i == -1);
            if (sj < programBlockIndexAt(startIndex + k)) { // k + i + 1 == k
                k = j;
            }
            f[j - k] = -1;
        } else {
            f[j - k] = i + 1;
        }
    }

    return startIndex + k;
}

bool RunSummary::determineRotationEquivalence(int index1, int index2, int len, int &offset) const {
    int ci1 = calculateCanonicalLoopIndex(index1, len);
    int ci2 = calculateCanonicalLoopIndex(index2, len);

    for (int i = len; --i >= 0; ) {
        if (programBlockIndexAt(ci1 + i) != programBlockIndexAt(ci2 + i)) {
            return false;
        }
    }

    int relIndex1 = ci1 - index1;
    int relIndex2 = ci2 - index2;
    offset = (relIndex1 + len - relIndex2) % len;

    return true;
}


int RunSummary::areLoopsRotationEqual(const RunBlock* block1, const RunBlock* block2,
                                      int &indexOffset) const {
    int index1 = block1->getSequenceIndex();
    int index2 = block2->getSequenceIndex();
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

void RunSummary::dumpRunBlockSequenceNode(const RunBlockSequenceNode* node, int level) const {
    for (int i = 0; i < level; i++) {
        std::cout << "  ";
    }
    std::cout << (node - _sequenceBlock)
    << " (" << node->getProgramBlockIndex() << ")" << std::endl;

    // Dump children
    if (node->_childIndex) {
        dumpRunBlockSequenceNode(_sequenceBlock + node->_childIndex, level + 1);
    }

    // Dump siblings
    if (node->_siblingIndex) {
        dumpRunBlockSequenceNode(_sequenceBlock + node->_siblingIndex, level);
    }
}

void RunSummary::dumpSequenceTree() const {
    dumpRunBlockSequenceNode(_sequenceBlock, 0);
}

void RunSummary::dumpCondensed() const {
    for (int i = 0; i < getNumRunBlocks(); i++) {
        const RunBlock* runBlock = runBlockAt(i);
        int sequenceIndex = runBlock->getSequenceIndex();

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
        int sequenceIndex = runBlock->getSequenceIndex();

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

            ProgramBlockIndex* programBlockP = _programBlockHistory + runBlock->getStartIndex();
            for (int k = 0; k < len; k++) {
                std::cout << " " << *(programBlockP++);
            }
            std::cout << std::endl;

            helperArray[numUnique++] = sequenceIndex;
            if (numUnique == 64) {
                return;
            }
        }
    }
}

void RunSummary::dump() const {
    ProgramBlockIndex* programBlockP = _programBlockHistory;
    const RunBlock* runBlockP = _runBlockHistory;
    int numPendingBlocks = 0;
    bool isLoop = false;

    while (programBlockP < _programBlockHistoryP) {
        if (--numPendingBlocks == 0) {
            std::cout << (isLoop ? "] " : ") ");
        }
        if (
            numPendingBlocks <= 0 &&
            runBlockP->getStartIndex() == (programBlockP - _programBlockHistory)
        ) {
            isLoop = runBlockP->isLoop();
            std::cout << "#" << runBlockP->getSequenceIndex()
            << (isLoop ? "[" : "(");
            numPendingBlocks = getRunBlockLength((int)(runBlockP - _runBlockHistory));

            runBlockP++;
        } else {
            std::cout << " ";
        }
        std::cout << (int)*programBlockP++;
    }
    std::cout << std::endl;
}
