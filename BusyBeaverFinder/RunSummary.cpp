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

void RunBlockSequenceNode::init(ProgramBlockIndex programBlockIndex) {
    _programBlockIndex = programBlockIndex;
    _childIndex = 0;
    _siblingIndex = 0;
}

void RunBlock::init(int startIndex, int sequenceIndex, bool isLoop) {
    _startIndex = startIndex;
    _sequenceIndex = sequenceIndex;
    _isLoop = isLoop;
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

void RunSummary::setCapacity(int capacity) {
    freeDynamicArrays();

    _programBlockHistory = new ProgramBlockIndex[capacity];
    _programBlockHistoryMaxP = _programBlockHistory + capacity;
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

    while (node->_siblingIndex) {
        node = _sequenceBlock + node->_siblingIndex;
        if (node->getProgramBlockIndex() == targetIndex) {
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

void RunSummary::createRunBlock(ProgramBlockIndex* startP, ProgramBlockIndex* endP, bool isLoop) {
    int sequenceIndex = getSequenceIndex(startP, endP);

    (_runBlockHistoryP++)->init((int)(startP - _programBlockHistory), sequenceIndex, isLoop);
}

int RunSummary::detectLoop() {
    ProgramBlockIndex* blockP = _programBlockHistoryP;

    while (--blockP >= _programBlockPendingP) {
        if (*blockP == *_programBlockHistoryP) {
            // Current program block already encountered, so we're in a loop
            return (int)(_programBlockHistoryP - blockP);
        }
    }

    // Not in loop yet
    return 0;
}

bool RunSummary::recordProgramBlock(ProgramBlockIndex blockIndex) {
//    std::cout << "recordProgramBlock #" << (int)blockIndex << std::endl;

    *_programBlockHistoryP = blockIndex;
    assert(_programBlockHistoryP != _programBlockHistoryMaxP);

    bool newRunBlocks = false;

    if (_loopP == nullptr) {
        int loopLen = detectLoop();
        if (loopLen > 0) {
//            std::cout << "Loop detected (len = " << loopLen << ")" << std::endl;
            ProgramBlockIndex* loopStartP = _programBlockHistoryP - loopLen;

            if (loopStartP != _programBlockPendingP) {
                createRunBlock(_programBlockPendingP, loopStartP, false);
            }
            createRunBlock(loopStartP, _programBlockHistoryP, true);

            _loopP = _programBlockHistoryP - loopLen;
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
        return (++runBlockP)->getStartIndex() - startIndex;
    }
}

void RunSummary::dump() {
    ProgramBlockIndex* programBlockP = _programBlockHistory;
    RunBlock* runBlockP = _runBlockHistory;
    int numPendingBlocks = 0;

    while (programBlockP < _programBlockHistoryP) {
        if (--numPendingBlocks == 0) {
            std::cout << ") ";
        }
        if (
            numPendingBlocks <= 0 &&
            runBlockP->getStartIndex() == (programBlockP - _programBlockHistory)
        ) {
            std::cout << "#" << runBlockP->getSequenceIndex() << "(";
            numPendingBlocks = getRunBlockLength((int)(runBlockP - _runBlockHistory));

            runBlockP++;
        } else {
            std::cout << " ";
        }
        std::cout << (int)*programBlockP++;
    }
    std::cout << std::endl;
}
