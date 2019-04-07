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

void RunBlockSequence::init(ProgramBlockIndex programBlockIndex) {
    _programBlockIndex = programBlockIndex;
    _children[0] = -1;
    _children[1] = -1;
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

    _numSequenceStarts = 0;
    _nextSequenceIndex = maxSequenceStarts;
}

RunBlockSequence* RunSummary::findSequenceStart(ProgramBlockIndex targetIndex) {
    RunBlockSequence* sequenceStartBlock = _sequenceBlock + _numSequenceStarts;

    while (sequenceStartBlock-- > _sequenceBlock) {
        if (sequenceStartBlock->getProgramBlockIndex() == targetIndex) {
            return sequenceStartBlock;
        }
    }

    // There's no sequence start yet for this program block. Create it
    assert(_numSequenceStarts < maxSequenceStarts);

    sequenceStartBlock = _sequenceBlock + _numSequenceStarts++;
    sequenceStartBlock->init(targetIndex);

    return sequenceStartBlock;
}

int RunSummary::getSequenceIndex(ProgramBlockIndex* startP, ProgramBlockIndex* endP) {
    ProgramBlockIndex* blockP = startP;
    RunBlockSequence* sequenceP = findSequenceStart(*blockP++);

    while (blockP != endP) {
        int* childIndex = sequenceP->_children + *blockP % 2;

        if (*childIndex == -1) {
            *childIndex = _nextSequenceIndex;
            sequenceP = &_sequenceBlock[_nextSequenceIndex++];
            sequenceP->init(*blockP);
        } else {
            sequenceP = &_sequenceBlock[*childIndex];
            assert(sequenceP->getProgramBlockIndex() == *blockP);
        }

        blockP++;
    }

    return (int)(sequenceP - _sequenceBlock);
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
    bool isLast = index == (getLength() - 1);
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
