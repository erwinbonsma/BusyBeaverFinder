//
//  HangExecutor.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#include "HangExecutor.h"

#include "HangDetector.h"
#include "ProgramBlock.h"

RunResult HangExecutor::executeBlock() {
    if (_block->isDelta()) {
        _data.delta(_block->getInstructionAmount());
    } else {
        if (!_data.shift(_block->getInstructionAmount())) {
            return RunResult::DETECTED_HANG;
        }
    }

    _numSteps += _block->getNumSteps();

    if (_block->isExit()) {
        return RunResult::SUCCESS;
    }

    _block = (_data.val() == 0) ? _block->zeroBlock() : _block->nonZeroBlock();
    if (!_block->isFinalized()) {
        return RunResult::PROGRAM_ERROR;
    }

    if (_numSteps >= _maxSteps) {
        return RunResult::ASSUMED_HANG;
    }

    return RunResult::UNKNOWN;
}

RunResult HangExecutor::executeWithoutHangDetection() {
    RunResult result;

    do {
        result = executeBlock();
    } while (result == RunResult::UNKNOWN);

    return result;
}

RunResult HangExecutor::execute(const InterpretedProgram* program) {
    _program = program;

    _runSummary[0].reset();
    _runSummary[1].reset();

    for (auto hangDetector : _hangDetectors) {
        hangDetector->reset();
    }

    const ProgramBlock *entryBlock = _program->getEntryBlock();
    _block = entryBlock;
    _numSteps = 0;

    while (true) {
        // Record block before executing it. This way, when signalling a loop exit, the value
        // that triggered this, which typically is zero, is still present in the data values.
        int numRunBlocks = _runSummary[0].getNumRunBlocks();
        if (_runSummary[0].recordProgramBlock((int)(_block - entryBlock))) {
            for (int i = numRunBlocks; i < _runSummary[0].getNumRunBlocks(); i++) {
                const RunBlock* runBlock = _runSummary[0].runBlockAt(i);
                _runSummary[1].recordProgramBlock(runBlock->getSequenceIndex());
            }

            if (!_runSummary[0].hasSpaceRemaining()) {
                // Stop hang detection as history buffer is full
                return executeWithoutHangDetection();
            }
        }

        RunResult result = executeBlock();
        if (result != RunResult::UNKNOWN) {
            return result;
        }

        if (_runSummary[0].isInsideLoop()) {
            bool loopContinues = _runSummary[0].loopContinues((int)(_block - entryBlock));
            for (auto hangDetector : _hangDetectors) {
                if (hangDetector->detectHang(loopContinues)) {
                    return RunResult::DETECTED_HANG;
                }
            }
        }
    }
}
