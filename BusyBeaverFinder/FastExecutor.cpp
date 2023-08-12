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

const int sentinelSize = 8;

FastExecutor::FastExecutor(int dataSize) {
    _dataBufSize = dataSize + 2 * sentinelSize;
    _data = new int[_dataBufSize];

    _minDataP = &_data[sentinelSize]; // Inclusive
    _maxDataP = _minDataP + dataSize; // Exclusive
    _midDataP = &_data[_dataBufSize / 2];
}

FastExecutor::~FastExecutor() {
    delete[] _data;
}

RunResult FastExecutor::execute(const ProgramBlock *programBlock) {
    _numSteps = 0;

    // Clear data
    memset(_data, 0, _dataBufSize * sizeof(int));

    _dataP = _midDataP;

    while (true) {
        int amount = programBlock->getInstructionAmount();

        if (programBlock->isDelta()) {
            *_dataP += amount;
        } else {
            _dataP += amount;
            if (_dataP < _minDataP || _dataP >= _maxDataP) {
                return RunResult::DATA_ERROR;
            }
        }

        _numSteps += programBlock->getNumSteps();
        if (_numSteps > _maxSteps) {
            return RunResult::ASSUMED_HANG;
        }

        if (programBlock->isExit()) {
            return RunResult::SUCCESS;
        }

        programBlock = (*_dataP == 0) ? programBlock->zeroBlock() : programBlock->nonZeroBlock();
        if (!programBlock->isFinalized()) {
            return RunResult::PROGRAM_ERROR;
        }
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
