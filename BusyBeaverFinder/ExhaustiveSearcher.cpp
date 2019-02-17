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

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, int dataSize) :
    _program(width, height),
    _data(dataSize),
    _cycleDetector(),
    _dataTracker(_data)
{
    initInstructionStack(width * height);

    // Init defaults
    _settings.maxSteps = 1024;
    _settings.initialHangSamplePeriod = 16;
    _settings.maxHangDetectAttempts = 4;
    _settings.testHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;
}

void ExhaustiveSearcher::configure(SearchSettings settings) {
    _settings = settings;
    reconfigure();
}

void ExhaustiveSearcher::reconfigure() {
    assert(isPowerOfTwo(_settings.initialHangSamplePeriod));
    _initialHangSampleMask = _settings.initialHangSamplePeriod - 1;

    // Determine the maximum sample period (it doubles after each failed attempt)
    int maxHangSamplePeriod = _settings.initialHangSamplePeriod;
    int i = _settings.maxHangDetectAttempts;
    while (--i > 0) {
        maxHangSamplePeriod <<= 1;
    }

    _data.setHangSamplePeriod(maxHangSamplePeriod);
    _cycleDetector.setHangSamplePeriod(maxHangSamplePeriod);
    _data.setStackSize(_settings.maxSteps + maxHangSamplePeriod);
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

void ExhaustiveSearcher::dumpInstructionStack() {
    std::cout << "Instruction stack: ";
    dumpInstructionStack(_instructionStack);
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _data.getSize()
    << ", MaxSteps = " << _settings.maxSteps
    << ", IniHangSamplePeriod = " << _settings.initialHangSamplePeriod
    << ", MaxDetectAttempts = " << _settings.maxHangDetectAttempts
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

bool ExhaustiveSearcher::periodicHangDetected() {
//    _cycleDetector.dump();
//    _data.dumpHangInfo();
//    _dataTracker.dump();

    if (_pp.p != _samplePp.p || _pp.dir != _samplePp.dir) {
        // Not back at the sample point, so not on a hang cycle with assumed period. Fail this
        // attempt and initiate a new one.
        _activeHangCheck = HangCheck::NONE;
        return false;
    }

    if (
        _data.getDataPointer() == _dataTracker.getNewSnapShot()->dataP &&
        !_data.significantValueChange()
    ) {
        SnapShotComparison result = _dataTracker.compareToSnapShot();
        if (result != SnapShotComparison::IMPACTFUL) {
            return true;
        }
    }
    else {
        if (_dataTracker.getOldSnapShot() != nullptr) {
            if (_dataTracker.periodicHangDetected()) {
                return true;
            } else {
                _activeHangCheck = HangCheck::NONE;
            }
        } else {
            _opsToWaitBeforePeriodicHangCheck = _cyclePeriod;
        }

        _dataTracker.captureSnapShot();
    }

    return false;
}

bool ExhaustiveSearcher::sweepHangDetected() {
    DataDirection extensionDir = DataDirection::NONE;
    if (_data.getDataPointer() == _data.getMinBoundP()) {
        // At left end of sequence
        extensionDir = DataDirection::LEFT;
    }
    else if (_data.getDataPointer() == _data.getMaxBoundP()) {
        // At right end of sequence
        extensionDir = DataDirection::RIGHT;
    }

    if (extensionDir != DataDirection::NONE && extensionDir != _prevExtensionDir) {
        _prevExtensionDir = extensionDir;
        _extensionCount++;
        if (_extensionCount == 3) {
//            std::cout << "Checking for sweep hang " << std::endl;
            if (_dataTracker.sweepHangDetected()) {
//                std::cout << "Sweep hang detected!" << std::endl;
//                _data.dump();
//                _dataTracker.dump();
                return true;
            }
            _remainingSweepHangDetectAttempts--;
            _extensionCount = 1; // Continue with next detection attempt
        }
        _dataTracker.captureSnapShot();
    }

    return false;
}

void ExhaustiveSearcher::initiateNewHangCheck() {
//    _program.dump();
//    _tracker->dumpStats();
//    _data.dumpHangInfo();
//    _data.dump();
//    _dataTracker.dump();
//    _cycleDetector.dump();

    if (_remainingPeriodicHangDetectAttempts > 0) {
        _remainingPeriodicHangDetectAttempts--;

        // Initiate new periodic hang check (maybe it was not stuck yet, or maybe the
        // previous sample period was too low to detect the period of the hang cycle)
        _activeHangCheck = HangCheck::PERIODIC;

        _samplePp = _pp;
        _cyclePeriod = _cycleDetector.getCyclePeriod();
        _opsToWaitBeforePeriodicHangCheck = _cyclePeriod;
//        std::cout << "period = " << _cyclePeriod << std::endl;
        _cycleDetector.clearInstructionHistory();
        _data.resetHangDetection();
        _dataTracker.reset();
        _dataTracker.captureSnapShot();

        _hangSampleMask = (_hangSampleMask << 1) | 1;
    }
    else if (_remainingPeriodicHangDetectAttempts == 0) {
        _remainingPeriodicHangDetectAttempts--;

        _activeHangCheck = HangCheck::SWEEP;

        _remainingSweepHangDetectAttempts = 3;
        _extensionCount = 0;
        _prevExtensionDir = DataDirection::NONE;
    }
}


void ExhaustiveSearcher::branch(int totalSteps, int depth) {
    ProgramPointer pp0 = _pp;
    Ins* insP = _pp.p + (int)_pp.dir;
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
        run(totalSteps, depth + 1);
        _pp = pp0;

        if (
            _searchMode == SearchMode::FIND_ONE ||
            (_searchMode == SearchMode::SUB_TREE && resuming)
        ) {
            break;
        }
    }
    _program.clearInstruction(insP);
    _instructionStack[depth] = Ins::UNSET;
}

void ExhaustiveSearcher::run(int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    _data.resetHangDetection();
    _dataTracker.reset();
    _cycleDetector.clearInstructionHistory();
    _samplePp.p = nullptr;

    _hangSampleMask = _initialHangSampleMask;
    _remainingPeriodicHangDetectAttempts = _settings.maxHangDetectAttempts;

    _activeHangCheck = HangCheck::NONE;

    while (1) { // Run until branch, termination or error
        Ins* insP;
        bool done = false;
        do { // Execute single step

            insP = _pp.p + (int)_pp.dir;

            Ins ins = _program.getInstruction(insP);
            switch (ins) {
                case Ins::DONE:
                    _tracker->reportDone(totalSteps + steps);
                    _data.undo(numDataOps);
                    return;
                case Ins::UNSET:
                    branch(totalSteps + steps, depth);
                    _data.undo(numDataOps);
                    return;
                case Ins::NOOP:
                    done = true;
                    break;
                case Ins::DATA:
                    numDataOps++;
                    switch (_pp.dir) {
                        case Dir::UP:
                            _data.inc();
                            break;
                        case Dir::DOWN:
                            _data.dec();
                            break;
                        case Dir::RIGHT:
                            if (! _data.shr()) {
                                _tracker->reportError();
                                _data.undo(numDataOps);
                                return;
                            }
                            break;
                        case Dir::LEFT:
                            if (! _data.shl()) {
                                _tracker->reportError();
                                _data.undo(numDataOps);
                                return;
                            }
                            break;
                    }
                    done = true;
                    break;
                case Ins::TURN:
                    if (_data.val() == 0) {
                        switch (_pp.dir) {
                            case Dir::UP: _pp.dir = Dir::LEFT; break;
                            case Dir::RIGHT: _pp.dir = Dir::UP; break;
                            case Dir::DOWN: _pp.dir = Dir::RIGHT; break;
                            case Dir::LEFT: _pp.dir = Dir::DOWN; break;
                        }
                    } else {
                        switch (_pp.dir) {
                            case Dir::UP: _pp.dir = Dir::RIGHT; break;
                            case Dir::RIGHT: _pp.dir = Dir::DOWN; break;
                            case Dir::DOWN: _pp.dir = Dir::LEFT; break;
                            case Dir::LEFT: _pp.dir = Dir::UP; break;
                        }
                    }
                    break;
            }
            if (_remainingPeriodicHangDetectAttempts >= 0) {
                _opsToWaitBeforePeriodicHangCheck--;
                if (_remainingPeriodicHangDetectAttempts > 0) {
                    _cycleDetector.recordInstruction((char)ins | (((char)_pp.dir) << 2));
                }
            }
        } while (!done);
        _pp.p = insP;
        steps++;

//        std::cout << "steps = " << steps << ", depth = " << depth << std::endl;

        if (
            _activeHangCheck == HangCheck::PERIODIC &&
            _opsToWaitBeforePeriodicHangCheck <= 0 &&
            periodicHangDetected()
        ) {
            _tracker->reportEarlyHang();
            if (!_settings.testHangDetection) {
                _data.undo(numDataOps);
                return;
            }
        }

        if (
            _activeHangCheck == HangCheck::SWEEP &&
            sweepHangDetected()
        ) {
            _tracker->reportEarlyHang();
            if (!_settings.testHangDetection) {
                _data.undo(numDataOps);
                return;
            }
        }

        if (! (steps & _hangSampleMask)) {
//            std::cout << "Monitor Hang Detection: Steps = " << (steps + totalSteps)
//            << ", active hang check = " << (int)_activeHangCheck
//            << ", _opsToWaitBeforePeriodicHangCheck = " << _opsToWaitBeforePeriodicHangCheck
//            << std::endl;

            if (steps + totalSteps >= _settings.maxSteps) {
                _tracker->reportHang();
                _data.undo(numDataOps);
                return;
            }

            if (_activeHangCheck == HangCheck::NONE) {
                initiateNewHangCheck();
            }
        }
    }
}

void ExhaustiveSearcher::search() {
    _resumeFrom = new Ins[1];
    _resumeFrom[0] = Ins::UNSET;

    _pp.p = _program.startProgramPointer();
    _pp.dir = Dir::UP;
    run(0, 0);
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;

    std::cout << "Resuming from: ";
    dumpInstructionStack(_resumeFrom);

    _pp.p = _program.startProgramPointer();
    _pp.dir = Dir::UP;
    run(0, 0);
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
