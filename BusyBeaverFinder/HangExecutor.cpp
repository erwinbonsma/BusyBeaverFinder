//
//  HangExecutor.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#include "HangExecutor.h"

#include "GliderHangDetector.h"
#include "IrregularSweepHangDetector.h"
#include "MetaPeriodicHangDetector.h"
#include "PeriodicSweepHangDetector.h"

#include "ProgramBlock.h"

HangExecutor::HangExecutor(int dataSize, int maxHangDetectionSteps) :
    _hangDetectionStart(0),
    _maxHangDetectionSteps(maxHangDetectionSteps),
    _data(dataSize),
    _runSummary()
{
    _hangDetectors.push_back(new PeriodicHangDetector(*this));
    _hangDetectors.push_back(new MetaPeriodicHangDetector(*this));
    _hangDetectors.push_back(new GliderHangDetector(*this));
    _hangDetectors.push_back(new PeriodicSweepHangDetector(*this));
    _hangDetectors.push_back(new IrregularSweepHangDetector(*this));

    _zArrayHelperBuf = new int[maxHangDetectionSteps / 2];
    _runSummary[0].setCapacity(maxHangDetectionSteps, _zArrayHelperBuf);
    _runSummary[1].setCapacity(maxHangDetectionSteps, _zArrayHelperBuf);
}

HangExecutor::~HangExecutor() {
    for (auto hangDetector : _hangDetectors) {
        delete hangDetector;
    }

    delete[] _zArrayHelperBuf;
}

HangType HangExecutor::detectedHangType() const {
    return _detectedHang ? _detectedHang->hangType() : HangType::NO_DATA_LOOP;
}

RunResult HangExecutor::executeBlock() {
//    _block->dump();

    if (!_block->isFinalized()) {
        return RunResult::PROGRAM_ERROR;
    }

    if (_block->isHang()) {
        return RunResult::DETECTED_HANG;
    }

    _numSteps += _block->getNumSteps();

    if (_block->isExit()) {
        return RunResult::SUCCESS;
    }

    if (_block->isDelta()) {
        _data.delta(_block->getInstructionAmount());
    } else {
        if (!_data.shift(_block->getInstructionAmount())) {
            return RunResult::DATA_ERROR;
        }
    }

    _block = (_data.val() == 0) ? _block->zeroBlock() : _block->nonZeroBlock();

    return RunResult::UNKNOWN;
}

RunResult HangExecutor::executeWithoutHangDetection(int stepLimit) {
    RunResult result = RunResult::UNKNOWN;

    while (result == RunResult::UNKNOWN && _numSteps < stepLimit) {
        result = executeBlock();
    }

    return result;
}

RunResult HangExecutor::executeWithHangDetection(int stepLimit) {
    _runSummary[0].reset();
    _runSummary[1].reset();

    for (auto hangDetector : _hangDetectors) {
        hangDetector->reset();
    }
    _detectedHang = nullptr;

    const ProgramBlock *entryBlock = _program->getEntryBlock();
    while (_numSteps < stepLimit) {
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
                return RunResult::UNKNOWN;
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
                    _detectedHang = hangDetector;
                    return RunResult::DETECTED_HANG;
                }
            }
        }
    }

    return RunResult::UNKNOWN;
}

RunResult HangExecutor::run() {
    RunResult result = executeWithoutHangDetection(_hangDetectionStart);
    _hangDetectionStart = 0; // Only used once
    if (result != RunResult::UNKNOWN) return result;

    result = executeWithHangDetection(std::min(_numSteps + _maxHangDetectionSteps, _maxSteps));
    if (result != RunResult::UNKNOWN) return result;

    result = executeWithoutHangDetection(_maxSteps);
    if (result != RunResult::UNKNOWN) return result;

    return RunResult::ASSUMED_HANG;
}

void HangExecutor::pop() {
    _executionStack.pop_back();
}

RunResult HangExecutor::execute(const InterpretedProgram* program) {
    if (_executionStack.size() > 0) {
        assert(program == _program);

        ExecutionStackFrame& frame = _executionStack.back();
        _numSteps = frame.numSteps;
        _block = frame.programBlock;
        _data.undo(frame.dataStackSize);
    } else {
        _program = program;

        _numSteps = 0;
        _data.reset();
        _block = _program->getEntryBlock();
    }

    RunResult result = run();

    // Push result on stack
    ExecutionStackFrame frame = {
        .programBlock = _block,
        .dataStackSize = _data.undoStackSize(),
        .numSteps = _numSteps
    };
    _executionStack.push_back(frame);

    return result;
}

RunResult HangExecutor::execute(std::string programSpec) {
    Program program = Program::fromString(programSpec);
    InterpretedProgramBuilder programBuilder = InterpretedProgramBuilder::fromProgram(program);

    std::cout << "Executing: " << programSpec << std::endl;

    return execute(&programBuilder);
}

void HangExecutor::dump() const {
    _runSummary[0].dump();
    _runSummary[1].dump();
}

void HangExecutor::dumpExecutionState() const {
    // TODO
}
