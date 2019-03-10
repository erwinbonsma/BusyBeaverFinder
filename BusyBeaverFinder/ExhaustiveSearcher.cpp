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

    _periodicHangDetector = new PeriodicHangDetector(*this);
    _regularSweepHangDetector = new RegularSweepHangDetector(*this);

    // Init defaults
    _settings.maxSteps = 1024;
    _settings.initialHangSamplePeriod = 16;
    _settings.maxPeriodicHangDetectAttempts = 5;
    _settings.maxRegularSweepHangDetectAttempts = 3;
    _settings.maxRegularSweepExtensionCount = 5;
    _settings.testHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;
    delete _periodicHangDetector;
    delete _regularSweepHangDetector;
}

void ExhaustiveSearcher::configure(SearchSettings settings) {
    _settings = settings;
    reconfigure();
}

void ExhaustiveSearcher::reconfigure() {
    assert(isPowerOfTwo(_settings.initialHangSamplePeriod));
    _initialHangSampleMask = _settings.initialHangSamplePeriod - 1;

    // Determine the maximum sample period (it doubles after each failed attempt)
    int maxHangSamplePeriod =
        _settings.initialHangSamplePeriod << _settings.maxPeriodicHangDetectAttempts;

    _data.setHangSamplePeriod(maxHangSamplePeriod);
    _cycleDetector.setHangSamplePeriod(maxHangSamplePeriod * 2);
    _data.setStackSize(_settings.maxSteps + maxHangSamplePeriod);

    _regularSweepHangDetector->setMaxSweepCount(_settings.maxRegularSweepExtensionCount);
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
    << ", MaxPeriodicHangDetectAttempts = " << _settings.maxPeriodicHangDetectAttempts
    << ", MaxRegularSweepDetectAttempts = " << _settings.maxRegularSweepHangDetectAttempts
    << ", MaxRegularSweepExtensionCount = " << _settings.maxRegularSweepExtensionCount
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dumpHangDetection() {
    _data.dumpHangInfo();
    _dataTracker.dump();
    if (_cycleDetectorEnabled) {
        _cycleDetector.dump();
    }
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
    _data.dump();
}

void ExhaustiveSearcher::initiateNewHangCheck() {
//    _program.dump();
//    _tracker->dumpStats();
//    _data.dumpHangInfo();
//    _data.dump();
//    _dataTracker.dump();
//    _cycleDetector.dump();

    int attempts = _numHangDetectAttempts;
    if (attempts < _settings.maxPeriodicHangDetectAttempts) {
        // Initiate new periodic hang check (maybe it was not stuck yet, or maybe the
        // previous sample period was too low to detect the period of the hang cycle)
        _activeHangCheck = _periodicHangDetector;

        _hangSampleMask = (_hangSampleMask << 1) | 1;
    } else {
        attempts -= _settings.maxPeriodicHangDetectAttempts;
        _cycleDetectorEnabled = false;

        if (attempts < _settings.maxRegularSweepHangDetectAttempts) {
            _activeHangCheck = _regularSweepHangDetector;
        } else {
            _activeHangCheck = nullptr;
        }
    }

    if (_activeHangCheck != nullptr) {
        _numHangDetectAttempts++;
        _activeHangCheck->start();
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
    _cycleDetectorEnabled = true;
    _cycleDetector.clearInstructionHistory();

    _hangSampleMask = _initialHangSampleMask;
    _numHangDetectAttempts = 0;
    _activeHangCheck = nullptr;

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
                    _lastTurnDp = _data.getDataPointer();
                    if (_data.val() == 0) {
                        switch (_pp.dir) {
                            case Dir::UP: _pp.dir = Dir::LEFT; break;
                            case Dir::RIGHT: _pp.dir = Dir::UP; break;
                            case Dir::DOWN: _pp.dir = Dir::RIGHT; break;
                            case Dir::LEFT: _pp.dir = Dir::DOWN; break;
                        }
                        if (_activeHangCheck != nullptr) {
                            _activeHangCheck->signalLeftTurn();
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
            if (_cycleDetectorEnabled) {
                _cycleDetector.recordInstruction(
                    (char)((insP - _program.getInstructionBuffer()) ^ (int)_pp.dir)
                );
            }
        } while (!done);
        _pp.p = insP;
        steps++;

//        std::cout << "steps = " << steps << ", depth = " << depth << std::endl;

        if (_activeHangCheck != nullptr) {
            HangDetectionResult result = _activeHangCheck->detectHang();

            switch (result) {
                case HangDetectionResult::FAILED:
                    _activeHangCheck = nullptr;
                    break;
                case HangDetectionResult::HANGING:
                    _tracker->reportEarlyHang();
                    if (!_settings.testHangDetection) {
                        _data.undo(numDataOps);
                        return;
                    }
                    break;
                case HangDetectionResult::ONGOING:
                    break;
            }
        }
        if (! (steps & _hangSampleMask)) {
//            std::cout << "Monitor Hang Detection: Steps = " << (steps + totalSteps)
//            << ", active hang check = " << (int)_activeHangCheck
//            << std::endl;

            if (steps + totalSteps >= _settings.maxSteps) {
                _tracker->reportHang();
                _data.undo(numDataOps);
                return;
            }

            if (_activeHangCheck == nullptr) {
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
