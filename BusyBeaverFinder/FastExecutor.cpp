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

#include "InterpretedProgram.h"
#include "ProgramBlock.h"

const int sentinelSize = 8;

FastExecutor::FastExecutor(int dataSize) {
    _dataBufSize = dataSize + 2 * sentinelSize;
    _data = new int[_dataBufSize];

    _minDataP = &_data[sentinelSize]; // Inclusive
    _maxDataP = _minDataP + dataSize; // Exclusive
    _midDataP = &_data[_dataBufSize / 2];

    _canResume = false;
}

FastExecutor::~FastExecutor() {
    delete[] _data;
}

RunResult FastExecutor::run() {
    _canResume = false;

    while (true) {
        if (!_block->isFinalized()) {
            _canResume = true;
            return RunResult::PROGRAM_ERROR;
        }
        if (_block->isHang()) {
            return RunResult::DETECTED_HANG;
        }

        _numSteps += _block->getNumSteps();
        if (_block->isExit()) {
            return RunResult::SUCCESS;
        }

        int amount = _block->getInstructionAmount();

        if (_block->isDelta()) {
            *_dataP += amount;
        } else {
            _dataP += amount;
            if (_dataP < _minDataP || _dataP >= _maxDataP) {
                return RunResult::DATA_ERROR;
            }
        }

        if (_numSteps > _maxSteps) {
            return RunResult::ASSUMED_HANG;
        }

        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();
    }
}

RunResult FastExecutor::execute(std::shared_ptr<const InterpretedProgram> program) {
    if (!_canResume) {
        // Start execution form the start
        _numSteps = 0;

        // Clear data
        memset(_data, 0, _dataBufSize * sizeof(int));

        _dataP = _midDataP;
        _block = program->getEntryBlock();
    }

    return run();
}

void FastExecutor::resumeFrom(const ProgramBlock* block, const Data& data, int numSteps) {
    // Copy the data
    memset(_data, 0, _dataBufSize * sizeof(int));
    DataPointer srcP = data.getMinDataP();
    int* dstP = _midDataP - (data.getMidDataP() - data.getMinDataP());
    while (srcP <= data.getMaxDataP()) {
        *dstP++ = *srcP++;
    }

    _block = block;
    _numSteps = numSteps;
    _dataP = _midDataP - (data.getMidDataP() - data.getDataPointer());

    _canResume = true;
}

void FastExecutor::dump() const {
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
