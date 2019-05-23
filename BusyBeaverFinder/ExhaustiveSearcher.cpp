//
//  ExhaustiveSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExhaustiveSearcher.h"

#include <iostream>
#include <assert.h>

#include "Utils.h"

Ins validInstructions[] = { Ins::NOOP, Ins::DATA, Ins::TURN };

Ins targetStack[] = {
    Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
    Ins::NOOP, Ins::TURN, Ins::UNSET
};

const ProgramPointer backtrackProgramPointer =
    ProgramPointer { .p = InstructionPointer { .col = -1, .row = -1 }, .dir = Dir::UP };

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, int dataSize) :
    _program(width, height),
    _data(dataSize),
    _runSummary(),
    _dataTracker(_data),
    _exitFinder(_program, _interpretedProgram),
    _fastExecutor(dataSize)
{
    initInstructionStack(width * height);

    _periodicHangDetector = new PeriodicHangDetector(*this);
    _metaPeriodicHangDetector = new MetaPeriodicHangDetector(*this);
    _sweepHangDetector = new SweepHangDetector(*this);
    _gliderHangDetector = new GliderHangDetector(*this);
    _zArrayHelperBuf = nullptr;

    _searchMode = SearchMode::FULL_TREE;

    // Init defaults
    _settings.maxSteps = 1024;
    _settings.maxHangDetectionSteps = _settings.maxSteps;
    _settings.maxHangDetectAttempts = 128;
    _settings.minWaitBeforeRetryingHangChecks = 16;
    _settings.testHangDetection = false;
    _settings.disableNoExitHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;
    delete _periodicHangDetector;
    delete _sweepHangDetector;
    delete[] _zArrayHelperBuf;
}

void ExhaustiveSearcher::configure(SearchSettings settings) {
    _settings = settings;
    reconfigure();
}

void ExhaustiveSearcher::reconfigure() {
    if (_zArrayHelperBuf != nullptr) {
        delete[] _zArrayHelperBuf;
    }
    _zArrayHelperBuf = new int[_settings.maxHangDetectionSteps / 2];

    _runSummary[0].setCapacity(_settings.maxHangDetectionSteps, _zArrayHelperBuf);
    _runSummary[1].setCapacity(_settings.maxHangDetectionSteps, _zArrayHelperBuf);
    _data.setStackSize(_settings.maxHangDetectionSteps);
}

void ExhaustiveSearcher::initInstructionStack(int size) {
    _instructionStack = new Ins[size];

    for (int i = size; --i >= 0; ) {
        _instructionStack[i] = Ins::UNSET;
    }
}

void ExhaustiveSearcher::dumpInstructionStack(Ins* stack) {
    while (*stack != Ins::UNSET) {
        std::cout << (int)*stack << ",";
        stack++;
    }
    std::cout << std::endl;
}

bool ExhaustiveSearcher::instructionStackEquals(Ins* reference) {
    Ins* p1 = _instructionStack;
    Ins* p2 = reference;

    while (*p1 != Ins::UNSET && *p2 != Ins::UNSET && *p1 == *p2) {
        p1++;
        p2++;
    }

    return (*p1 == *p2);
}

bool ExhaustiveSearcher::atTargetProgram() {
    return instructionStackEquals(targetStack);
}

void ExhaustiveSearcher::dumpInstructionStack() {
    dumpInstructionStack(_instructionStack);
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _data.getSize()
    << ", MaxSteps = " << _settings.maxHangDetectionSteps << "/" << _settings.maxSteps
    << ", MaxHangDetectAttempts = " << _settings.maxHangDetectAttempts
    << ", MinWaitBeforeRetryingHangChecks = " << _settings.minWaitBeforeRetryingHangChecks
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dumpHangDetection() {
    _data.dumpHangInfo();
    _dataTracker.dump();

    std::cout << "Run summary: ";
    _runSummary[0].dump();
    _runSummary[0].dumpCondensed();

    std::cout << "Meta-run summary: ";
    _runSummary[1].dump();
    _runSummary[1].dumpCondensed();
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
    _data.dump();
}

void ExhaustiveSearcher::setProgressTracker(ProgressTracker* tracker) {
    _tracker = tracker;
    _fastExecutor.setProgressTracker(tracker);
}

HangDetector* ExhaustiveSearcher::initiateNewHangCheck() {
//    std::cout << "Initiating new hang check @ " << _numSteps << std::endl;
//    dumpHangDetection();

    if (_numHangDetectAttempts == _settings.maxHangDetectAttempts) {
        // Signal end of all checks
        _numHangDetectAttempts = -1;
        return nullptr;
    }

    HangDetector* newCheck = nullptr;

    switch (_numHangDetectAttempts % 3) {
        case 0: {
            if (_runSummary[0].getNumProgramBlocks() < _waitBeforeRetryingHangChecks) {
                // Wait before initiating a new hang check. Too quickly trying the same hang checks
                // in succession could just be a waste of CPU cycles.
                return nullptr;
            }
            _waitBeforeRetryingHangChecks = (
                _runSummary[0].getNumProgramBlocks() +
                _settings.minWaitBeforeRetryingHangChecks
            );
            // Alternate between both types of periodic hang detectors
            if (_numHangDetectAttempts % 6 == 0) {
                newCheck = _periodicHangDetector;
            } else {
                newCheck = _metaPeriodicHangDetector;
            }
            break;
        }
        case 1: newCheck = _sweepHangDetector; break;
        case 2: newCheck = _gliderHangDetector; break;
    }

    assert(newCheck != nullptr);
    _numHangDetectAttempts++;

    return newCheck;
}

void ExhaustiveSearcher::branch(int depth) {
    ProgramPointer pp0 = _pp;
    InstructionPointer insP = nextInstructionPointer(_pp);
    bool resuming = *_resumeFrom != Ins::UNSET;

    for (int i = 0; i < 3; i++) {
        Ins ins = validInstructions[i];

        if (resuming) {
            if (ins == *_resumeFrom) {
                _resumeFrom++;
            } else {
                continue;
            }
        }

        _program.setInstruction(insP, ins);
        _instructionStack[depth] = ins;
//        if (atTargetProgram()) {
//            std::cout << "At target!" << std::endl;
//        }
        run(depth + 1);
        _pp = pp0;

        if (
            _searchMode == SearchMode::FIND_ONE ||
            (_searchMode == SearchMode::SUB_TREE && resuming)
        ) {
            break;
        }

        // Clear so that search continues after resuming
        resuming = false;
    }
    _program.clearInstruction(insP);
    _instructionStack[depth] = Ins::UNSET;
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocks() {
//    _program.dump();
//    _interpretedProgram.dump();

    HangDetectionResult result = HangDetectionResult::ONGOING;
    HangDetector* hangCheck = nullptr;
    ProgramBlock* entryBlock = _interpretedProgram.getEntryBlock();

    _data.resetHangDetection();
    _dataTracker.reset();
    _runSummary[0].reset();
    _runSummary[1].reset();

    // Wait a bit before the initial check, so that there is at least a bit of program block
    // history available. Otherwise it can take unnecessarily long for a simple periodic hang to be
    // detected.
    _waitBeforeRetryingHangChecks = 4;

    _numHangDetectAttempts = 0;

    while (_block->isFinalized()) {
        // Record block before executing it. This way, when signalling a loop exit, the value
        // that triggered this, which typically is zero, is still present in the data values.
        if (_numHangDetectAttempts >= 0) {
            // Only track execution while hang detection is still active
            bool wasInLoop = _runSummary[0].isInsideLoop();
            int numRunBlocks = _runSummary[0].getNumRunBlocks();

            if (_runSummary[0].recordProgramBlock((int)(_block - entryBlock))) {
                for (int i = numRunBlocks; i < _runSummary[0].getNumRunBlocks(); i++) {
                    RunBlock* runBlock = _runSummary[0].runBlockAt(i);
                    _runSummary[1].recordProgramBlock(runBlock->getSequenceIndex());
                }

                if (!_runSummary[0].hasSpaceRemaining()) {
                    // Disable hang detection as history buffer is full
                    hangCheck = nullptr;
                    _numHangDetectAttempts = -1;
                }
            }

            if (hangCheck != nullptr) {
                if (wasInLoop) {
                    if (!_runSummary[0].isInsideLoop()) {
                        result = hangCheck->signalLoopExit();
                    }
                    else if (_runSummary[0].isAtStartOfLoop()) {
                        result = hangCheck->signalLoopIteration();
                    }
                } else {
                    result = hangCheck->signalLoopStartDetected();
                }
            } else if (_numHangDetectAttempts >= 0) {
                hangCheck = initiateNewHangCheck();
                if (hangCheck != nullptr) {
                    result = hangCheck->start();
                }
            }

            if (result == HangDetectionResult::FAILED) {
                hangCheck = nullptr;
            }
            else if (result == HangDetectionResult::HANGING) {
                _tracker->reportDetectedHang(hangCheck->hangType());
                if (!_settings.testHangDetection) {
                    return backtrackProgramPointer;
                }
            }
        }

        int amount = _block->getInstructionAmount();
        if (_block->isDelta()) {
            if (amount > 0) {
                while (amount-- > 0) {
                    _data.inc();
                }
            } else {
                while (amount++ < 0) {
                    _data.dec();
                }
            }
        } else {
            if (amount > 0) {
                while (amount-- > 0) {
                    if (!_data.shr()) {
                        _tracker->reportError();
                        return backtrackProgramPointer;
                    }
                }
            } else {
                while (amount++ < 0) {
                    if (!_data.shl()) {
                        _tracker->reportError();
                        return backtrackProgramPointer;
                    }
                }
            }
        }

        _numSteps += _block->getNumSteps();

        if (_numSteps >= _settings.maxHangDetectionSteps) {
            if (_numSteps >= _settings.maxSteps) {
                _tracker->reportAssumedHang();
            } else {
                _fastExecutor.execute(_interpretedProgram.getEntryBlock(), _settings.maxSteps);
            }
            return backtrackProgramPointer;
        }

        _block = (_data.val() == 0) ? _block->zeroBlock() : _block->nonZeroBlock();
    }

    _interpretedProgram.enterBlock(_block);
    return _interpretedProgram.getStartProgramPointer(_block, _program);
}

void ExhaustiveSearcher::run(int depth) {
    DataOp* initialDataUndoP = _data.getUndoStackPointer();
    int initialSteps = _numSteps;

    _interpretedProgram.push();

//    _program.dump();
//    std::cout << std::endl;

    while (1) { // Run until branch, termination or error
processInstruction:
        InstructionPointer insP = nextInstructionPointer(_pp);

        switch (_program.getInstruction(insP)) {
            case Ins::DONE:
                _tracker->reportDone(_numSteps + 1);
                goto backtrack;
            case Ins::UNSET:
                branch(depth);
                goto backtrack;
            case Ins::NOOP:
                break;
            case Ins::DATA:
                switch (_pp.dir) {
                    case Dir::UP:
                        _data.inc();
                        _interpretedProgram.setInstruction(true);
                        _interpretedProgram.incAmount();
                        break;
                    case Dir::DOWN:
                        _data.dec();
                        _interpretedProgram.setInstruction(true);
                        _interpretedProgram.decAmount();
                        break;
                    case Dir::RIGHT:
                        if (! _data.shr()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _interpretedProgram.setInstruction(false);
                        _interpretedProgram.incAmount();
                        break;
                    case Dir::LEFT:
                        if (! _data.shl()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _interpretedProgram.setInstruction(false);
                        _interpretedProgram.decAmount();
                        break;
                }
                break;
            case Ins::TURN:
                if (_data.val() == 0) {
                    _pp.dir = (Dir)(((int)_pp.dir + 3) % 4);
                } else {
                    _pp.dir = (Dir)(((int)_pp.dir + 1) % 4);
                }
                if (_interpretedProgram.isInstructionSet()) {
                    _block = _interpretedProgram.finalizeBlock(_pp.p);

                    // Check if it is possible to exit
                    if (
                        _block != nullptr &&
                        !_settings.disableNoExitHangDetection &&
                        !_exitFinder.canExitFrom(_block)
                    ) {
                        _tracker->reportDetectedHang(HangType::NO_EXIT);
                        if (!_settings.testHangDetection) {
                            goto backtrack;
                        }
                    }

                    _block = _interpretedProgram.enterBlock(
                        _pp.p,
                        _data.val()==0 ? TurnDirection::COUNTERCLOCKWISE : TurnDirection::CLOCKWISE
                    );

                    if (_block != nullptr) {
                        _pp = executeCompiledBlocks();
                        if (_pp.p.col == -1) {
                            goto backtrack;
                        }
                    }
                }
                goto processInstruction;
        }

        if (_interpretedProgram.incSteps() > 64) {
            _tracker->reportDetectedHang(HangType::NO_DATA_LOOP);
            if (!_settings.testHangDetection) {
                goto backtrack;
            }
        }

        _pp.p = insP;
        _numSteps++;

        // Needed when hang detection is tested. TODO: Find out why.
        if (_numSteps >= _settings.maxSteps) {
            _tracker->reportAssumedHang();
            goto backtrack;
        }

//        std::cout << "steps = " << steps << ", depth = " << depth << std::endl;
    }
backtrack:
    _data.undo(initialDataUndoP);
    _numSteps = initialSteps;
    _interpretedProgram.pop();
}

void ExhaustiveSearcher::search() {
    _resumeFrom = new Ins[1];
    _resumeFrom[0] = Ins::UNSET;

    _pp.p = _program.getStartProgramPointer();
    _pp.dir = Dir::UP;
    _numSteps = 0;
    run(0);
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;

    std::cout << "Resuming from: ";
    dumpInstructionStack(_resumeFrom);

    _pp.p = _program.getStartProgramPointer();
    _pp.dir = Dir::UP;
    _numSteps = 0;
    run(0);
}

void ExhaustiveSearcher::searchSubTree(Ins* resumeFrom) {
    _searchMode = SearchMode::SUB_TREE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne() {
    _searchMode = SearchMode::FIND_ONE;
    search();
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne(Ins* resumeFrom) {
    _searchMode = SearchMode::FIND_ONE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}
