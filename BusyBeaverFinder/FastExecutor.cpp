//
//  FastExecutor.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 22/05/19.
//  Copyright © 2019 Erwin Bonsma
//

#include "FastExecutor.h"

#include <string.h>
#include <iostream>

#include "ProgramBlock.h"
#include "ProgressTracker.h"

const int sentinelSize = 8;

FastExecutor::FastExecutor(int dataSize) {
    int bufSize = dataSize + 2 * sentinelSize;
    _data = new int[bufSize];

    _minDataP = &_data[sentinelSize]; // Inclusive
    _maxDataP = _minDataP + dataSize; // Exclusive
    _midDataP = &_data[bufSize / 2];
}

FastExecutor::~FastExecutor() {
    delete[] _data;
}

void FastExecutor::execute(ProgramBlock *programBlock, int maxSteps) {
    int numSteps = 0;

    _tracker->reportFastExecution();

    // Clear data
    memset(_data, 0, (_maxDataP - _minDataP) * sizeof(int));

    _dataP = _midDataP;

    while (numSteps < maxSteps && programBlock != nullptr && programBlock->isFinalized()) {
        int amount = programBlock->getInstructionAmount();

        if (programBlock->isDelta()) {
            *_dataP += amount;
        } else {
            _dataP += amount;
            if (_dataP < _minDataP || _dataP >= _maxDataP) {
                _tracker->reportError();
                return;
            }
        }

        numSteps += programBlock->getNumSteps();

        programBlock = (*_dataP == 0) ? programBlock->zeroBlock() : programBlock->nonZeroBlock();
    }

    if (numSteps >= maxSteps) {
        _tracker->reportAssumedHang();
    } else {
        _tracker->reportLateEscape(numSteps);
    }
}

void FastExecutor::dump() {
    // Find end
    int *max = _maxDataP - 1;
    while (max > _dataP && *max == 0) {
        max--;
    }
    // Find start
    int *p = _minDataP;
    while (p < _dataP && *p == 0) {
        p++;
    }

    std::cout << "Data: ";
    while (1) {
        if (p == _dataP) {
            std::cout << "[" << *p << "]";
        } else {
            std::cout << *p;
        }
        if (p < max) {
            p++;
            std::cout << " ";
        } else {
            break;
        }
    }
    std::cout << std::endl;
}
