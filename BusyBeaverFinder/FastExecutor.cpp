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

constexpr int maxShiftSize = 8;
constexpr int loopUnrollCount = 8;
constexpr int sentinelSize = maxShiftSize * loopUnrollCount;

FastExecutor::FastExecutor(int dataSize)
    : _dataBufSize(dataSize + 2 * sentinelSize), _data(_dataBufSize)
{
    _minDataP = &_data[sentinelSize]; // Inclusive
    _maxDataP = _minDataP + dataSize; // Exclusive
    _midDataP = &_data[_dataBufSize / 2];

    _canResume = false;
}

void FastExecutor::resetData() {
    _data.clear();
    _data.resize(_dataBufSize);
}

// Executes multiple iterations of the loop in between checking the two "slow" exit criteria:
// Too close to the limit of the data tape or exceeded the maximum number of steps.
//
// Note: This optimization cannot be achieved by a compiler loop unroll, as they compiler cannot
// know that the "slow conditions" can be checked less frequently.
//
// Reducing the number of checks speeds up execution. The only drawback is that it may execute
// slightly more steps than needed. However, the size of the data tape is chosen such that it
// will still always remain within allocated memory. The execution overhead of this slight
// overshoot is negligible compared to the gains when searching for long-running programs.
//
// Returns true if execution terminated because a slow criteria was violated, and false otherwise
// (when a terminating instruction was encountered).
bool FastExecutor::fastRun() {
    int amount;
    while (_dataP >= _minDataP && _dataP < _maxDataP && _numSteps <= _maxSteps) {
        // Step 1
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 2
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 3
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 4
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 5
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 6
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 7
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

        // Step 8
        if (_block->interruptsRun()) return false;
        _numSteps += _block->getNumSteps();
        amount = _block->getInstructionAmount();
        if (_block->isDelta()) { *_dataP += amount; } else { _dataP += amount; }
        _block = (*_dataP == 0) ? _block->zeroBlock() : _block->nonZeroBlock();
    }
    return true;
}

RunResult FastExecutor::run() {
    _canResume = false;

    if (fastRun()) {
        if (_numSteps > _maxSteps) {
            return RunResult::ASSUMED_HANG;
        }
        if (_dataP < _minDataP || _dataP >= _maxDataP) {
            return RunResult::DATA_ERROR;
        }

        assert(false);
    } else {
        if (!_block->isFinalized()) {
            _canResume = true;
            return RunResult::PROGRAM_ERROR;
        }
        if (_block->isHang()) {
            return RunResult::DETECTED_HANG;
        }
        if (_block->isExit()) {
            _numSteps += _block->getNumSteps();
            return RunResult::SUCCESS;
        }

        assert(false);
    }
}

RunResult FastExecutor::execute(std::shared_ptr<const InterpretedProgram> program) {
    if (!_canResume) {
        // Start execution from the start
        _numSteps = 0;

        resetData();

        _dataP = _midDataP;
        _block = program->getEntryBlock();
    }

    return run();
}

void FastExecutor::resumeFrom(const ProgramBlock* block, const Data& data, int numSteps) {
    // Copy the data
    resetData();
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
