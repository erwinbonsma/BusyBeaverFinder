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

    _noExitHangDetector = new NoExitHangDetector(*this);
    _periodicHangDetector = new PeriodicHangDetector(*this);
    _regularSweepHangDetector = new RegularSweepHangDetector(*this);

    // Init defaults
    _settings.maxSteps = 1024;
    _settings.initialHangSamplePeriod = 16;
    _settings.maxPeriodicHangDetectAttempts = 5;
    _settings.maxRegularSweepHangDetectAttempts = 3;
    _settings.maxRegularSweepExtensionCount = 5;
    _settings.testHangDetection = false;
    _settings.disableNoExitHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;
    delete _noExitHangDetector;
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

bool ExhaustiveSearcher::instructionStackEquals(Ins* reference) {
    Ins* p1 = _instructionStack;
    Ins* p2 = reference;

    while (*p1 != Ins::UNSET && *p2 != Ins::UNSET && *p1 == *p2) {
        p1++;
        p2++;
    }

    return (*p1 == *p2);
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

    assert(_activeHangCheck == nullptr);
//    std::cout << "Initiating Check: Step = " << _numSteps
//    << ", numAttempts = " << attempts << std::endl;

    if (!_settings.disableNoExitHangDetection) {
        if (attempts == 0) {
            _activeHangCheck = _noExitHangDetector;
        } else {
            attempts--;
        }
    }

    if (_activeHangCheck == nullptr) {
        if (attempts < _settings.maxPeriodicHangDetectAttempts) {
            // Initiate new periodic hang check (maybe it was not stuck yet, or maybe the
            // previous sample period was too low to detect the period of the hang cycle)
            _activeHangCheck = _periodicHangDetector;

            // Double period after each failed attempt
            _periodicHangDetector->setMinRecordedInstructions(
                _settings.initialHangSamplePeriod << attempts
            );
            _cycleDetectorEnabled = true;
        } else {
            attempts -= _settings.maxPeriodicHangDetectAttempts;
            _cycleDetectorEnabled = false;
        }
    }

    if (_activeHangCheck == nullptr && attempts < _settings.maxRegularSweepHangDetectAttempts) {
        _activeHangCheck = _regularSweepHangDetector;
    }

    if (_activeHangCheck != nullptr) {
        _numHangDetectAttempts++;
        _activeHangCheck->start();
    } else {
        // Signal end of all checks
        _numHangDetectAttempts = -1;
    }
}

void ExhaustiveSearcher::branch(int depth) {
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
        run(depth + 1);
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

void ExhaustiveSearcher::run(int depth) {
    int numDataOps = 0;
    int initialSteps = _numSteps;

    _data.resetHangDetection();
    _dataTracker.reset();
    _cycleDetectorEnabled = false;

    _numHangDetectAttempts = 0;
    _activeHangCheck = nullptr;

    while (1) { // Run until branch, termination or error
        InstructionPointer insP;
        bool done = false;
        do { // Execute single step

            insP = _pp.p + (int)_pp.dir;

            Ins ins = _program.getInstruction(insP);
            switch (ins) {
                case Ins::DONE:
                    _tracker->reportDone(_numSteps);
                    _data.undo(numDataOps);
                    _numSteps = initialSteps;
                    return;
                case Ins::UNSET:
                    branch(depth);
                    _data.undo(numDataOps);
                    _numSteps = initialSteps;
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
                                _numSteps = initialSteps;
                                return;
                            }
                            break;
                        case Dir::LEFT:
                            if (! _data.shl()) {
                                _tracker->reportError();
                                _data.undo(numDataOps);
                                _numSteps = initialSteps;
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
        _numSteps++;

//        std::cout << "steps = " << steps << ", depth = " << depth << std::endl;

        if (_activeHangCheck != nullptr) {
            HangDetectionResult result = _activeHangCheck->detectHang();

            switch (result) {
                case HangDetectionResult::FAILED:
                    _activeHangCheck = nullptr;
                    break;
                case HangDetectionResult::HANGING:
                    _tracker->reportDetectedHang(_activeHangCheck->hangType());
                    if (!_settings.testHangDetection) {
                        _data.undo(numDataOps);
                        _numSteps = initialSteps;
                        return;
                    }
                    break;
                case HangDetectionResult::ONGOING:
                    break;
            }
        } else if (_numHangDetectAttempts != -1) {
            initiateNewHangCheck();
        }

        if (_numSteps >= _settings.maxSteps) {
            _tracker->reportAssumedHang();
            _data.undo(numDataOps);
            _numSteps = initialSteps;
            return;
        }
    }
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
