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

// Checks if the current position is one where the sweep reverses, even though the data pointer is
// not at the bounds of the data store.
bool ExhaustiveSearcher::isPossibleMidSweepPoint() {
    if (_extensionCount != 1 || _data.val() != 0) {
        return false;
    }

    int *dp = _data.getDataPointer();
    if (_prevExtensionDir == DataDirection::LEFT) {
        if (dp < _data.getMinBoundP()) {
            // Zero value is outside sequence, extending the sequence at its first end point
            return false;
        }
        if ((dp - _data.getMinBoundP()) < 8) {
            // The sequence is too short.
            return false;
        }
    } else {
        if (dp > _data.getMaxBoundP()) {
            return false;
        }
        if ((_data.getMaxBoundP() - dp) < 8) {
            return false;
        }
    }

    long dataIndex = _data.getDataPointer() - _data.getDataBuffer();

    if (_dataTracker.getNewSnapShot()->buf[dataIndex] != 0) {
        // The value just became zero. It should have been zero to be a possible turning point.
        return false;
    }

    _sweepMidTurningPoint = _data.getDataPointer();
    _sweepMidTurningDir = (_prevExtensionDir == DataDirection::LEFT)
        ? DataDirection::RIGHT
        : DataDirection::LEFT;
    return true;
}

// Should be invoked after a full sweep has been completed. It returns "true" if the data value
// changes are diverging and therefore, the program is considered hanging.
bool ExhaustiveSearcher::isSweepDiverging() {
//    std::cout << "Checking for sweep hang " << std::endl;
//    _data.dump();
//    _dataTracker.dump();

    if (_sweepMidTurningPoint != nullptr) {
        long dataIndex = _sweepMidTurningPoint - _data.getDataBuffer();
        if (
            *_sweepMidTurningPoint != 0 ||
            _dataTracker.getNewSnapShot()->buf[dataIndex] != 0
        ) {
            // The mid-turning point is not zero anymore
            return false;
        }
    }

    if (_pp.p != _sweepStartPp.p || _pp.dir != _sweepStartPp.dir) {
        // Current program location differs from program location at start of sweep
        return false;
    }

    return _dataTracker.sweepHangDetected();
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
    else if (_sweepMidTurningPoint == nullptr && isPossibleMidSweepPoint()) {
        // Possible mid-sequence turning point
        extensionDir = _sweepMidTurningDir;
    }

    // The mid-turning point should not be crossed
    if (_sweepMidTurningPoint != nullptr) {
        int* dp = _data.getDataPointer();
        if (
            (_sweepMidTurningDir == DataDirection::RIGHT && dp > _sweepMidTurningPoint) ||
            (_sweepMidTurningDir == DataDirection::LEFT && dp < _sweepMidTurningPoint)
        ) {
//            std::cout << "Crossed the mid-turning point" << std::endl;
            _remainingSweepHangDetectAttempts--;
            _sweepMidTurningPoint = nullptr;
            // Start new detection attempt afresh
            _extensionCount = 0;
            return false;
        }
    }

    if (extensionDir != DataDirection::NONE && extensionDir != _prevExtensionDir) {
//        std::cout << "Sweep endpoint detected" << std::endl;
//        _program.dump(_pp.p);

        _prevExtensionDir = extensionDir;
        _extensionCount++;
        if (_extensionCount == 1) {
            _sweepStartPp = _pp;
        }
        else if (_extensionCount == 3) {
            if (isSweepDiverging()) {
//                std::cout << "Sweep hang detected!" << std::endl;
//                _data.dump();
//                _dataTracker.dump();
                return true;
            }
            if (_remainingSweepHangDetectAttempts-- > 0) {
                _sweepMidTurningPoint = nullptr;
                // Continue next detection attempt starting from current end point
                _extensionCount = 1;
                _sweepStartPp = _pp;
            } else {
                _activeHangCheck = HangCheck::NONE;
            }
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
        _remainingPeriodicHangDetectAttempts = -1; // Ensure this block is only executed once

        _activeHangCheck = HangCheck::SWEEP;

        _remainingSweepHangDetectAttempts = 3;
        _extensionCount = 0;
        _prevExtensionDir = DataDirection::NONE;
        _sweepMidTurningPoint = nullptr;
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
                    _cycleDetector.recordInstruction(
                        (char)((insP - _program.getInstructionBuffer()) ^ (int)_pp.dir)
                    );
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

    _pp.p = _program.getStartProgramPointer();
    _pp.dir = Dir::UP;
    run(0, 0);
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;

    std::cout << "Resuming from: ";
    dumpInstructionStack(_resumeFrom);

    _pp.p = _program.getStartProgramPointer();
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
