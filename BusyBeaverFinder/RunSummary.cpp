//
//  RunSummary.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "RunSummary.h"

#include <iostream>

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

    assert(_runBlockHistoryP < _runBlockHistoryThresholdP);
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

            _loopP = _programBlockHistoryP - loopPeriod;
            _programBlockPendingP = nullptr;
            newRunBlocks = true;
        }
    } else {
        if (*++_loopP != *_programBlockHistoryP) {
//            std::cout << "Loop exited!" << std::endl;
            // Loop is broken
            _programBlockPendingP = _programBlockHistoryP;
            _loopP = nullptr;
        }
    }

    _programBlockHistoryP++;

    return newRunBlocks;
}

int RunSummary::getRunBlockLength(int index) {
    bool isLast = index == (getNumRunBlocks() - 1);
    RunBlock* runBlockP = _runBlockHistory + index;
    int startIndex = runBlockP->getStartIndex();

    if (isLast) {
        if (_programBlockPendingP != nullptr) {
            // Last run is completed. There are still some unallocated program blocks though
            return (int)(_programBlockPendingP - _programBlockHistory) - startIndex;
        } else {
            return (int)(_programBlockHistoryP - _programBlockHistory) - startIndex;
        }
    } else {
        return (runBlockP + 1)->getStartIndex() - startIndex;
    }
}

bool RunSummary::isAtStartOfLoop() {
    assert(_loopP != nullptr);
    assert(_programBlockPendingP == nullptr);

    int startIndex = (_runBlockHistoryP - 1)->getStartIndex();
    int loopLength = (int)(_programBlockHistoryP - _programBlockHistory) - startIndex;

    return (loopLength % getLoopPeriod() == 0);
}

void RunSummary::dumpRunBlockSequenceNode(RunBlockSequenceNode* node, int level) {
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

void RunSummary::dumpSequenceTree() {
    dumpRunBlockSequenceNode(_sequenceBlock, 0);
}

void RunSummary::dumpCondensed() {
    for (int i = 0; i < getNumRunBlocks(); i++) {
        RunBlock* runBlock = runBlockAt(i);
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
        RunBlock* runBlock = runBlockAt(i);
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

void RunSummary::dump() {
    ProgramBlockIndex* programBlockP = _programBlockHistory;
    RunBlock* runBlockP = _runBlockHistory;
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
