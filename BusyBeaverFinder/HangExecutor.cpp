//
//  HangExecutor.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 12/08/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//
#include "HangExecutor.h"

#include "MetaLoopHangDetector.h"
#include "PeriodicHangDetector.h"

#include "ProgramBlock.h"

HangExecutor::HangExecutor(int dataSize, int maxHangDetectionSteps) :
    _hangDetectionStart(0),
    _maxHangDetectionSteps(maxHangDetectionSteps),
    _data(dataSize),
    _zArrayHelperBuf(maxHangDetectionSteps / 2),
    _runSummary(_runHistory, _zArrayHelperBuf.data()),
    _metaRunSummary(_runSummary.getRunBlocks(), _zArrayHelperBuf.data()),
    _metaMetaRunSummary(_metaRunSummary.getRunBlocks(), _zArrayHelperBuf.data()),
    _runBlockTransitions(_runSummary)
{
    _runSummary.setIdentifyShortLoops(true);
    setMaxSteps(maxHangDetectionSteps);
}

void HangExecutor::addDefaultHangDetectors() {
//    _hangDetectors.push_back(std::make_shared<PeriodicHangDetector>(*this));
    _hangDetectors.push_back(std::make_shared<MetaLoopHangDetector>(*this));
}

void HangExecutor::resetHangDetection() {
    _runHistory.clear();
    _runSummary.reset();
    _metaRunSummary.reset();
    _metaMetaRunSummary.reset();
    _runBlockTransitions.reset();

    for (auto hangDetector : _hangDetectors) {
        hangDetector->reset();
    }
    _detectedHang = nullptr;
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
    resetHangDetection();

    while (_numSteps < stepLimit) {
        // Record block before executing it. This way, when signalling a loop exit, the value
        // that triggered this, which typically is zero, is still present in the data values.
        _runHistory.push_back(_block);
        bool runBlockAdded = _runSummary.processNewRunUnits();
        if (runBlockAdded) {
            _runBlockTransitions.processNewRunBlocks();
            if (_metaRunSummary.processNewRunUnits()) {
                _metaMetaRunSummary.processNewRunUnits();
            }
        }

        RunResult result = executeBlock();
        if (result != RunResult::UNKNOWN) {
            return result;
        }

        if (_runSummary.isInsideLoop()) {
            _loopRunState = (runBlockAdded
                             ? LoopRunState::STARTED
                             : (_runSummary.loopContinues(_block->getStartIndex())
                                ? LoopRunState::RUNNING
                                : LoopRunState::ENDED));
//            if (!loopContinues) {
//                _data.dump();
//                _runSummary.dumpCondensed(true);
//            }
//            std::cout << "loopPhase = " << static_cast<int>(loopPhase) << std::endl;
            for (auto hangDetector : _hangDetectors) {
                if (hangDetector->detectHang()) {
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
//    programBuilder.dump();

    return execute(&programBuilder);
}

void HangExecutor::dump() const {
    std::cout << "#Steps = " << numSteps() << std::endl;
    dumpExecutionState();
}
