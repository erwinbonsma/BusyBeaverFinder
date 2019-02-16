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

Op validOps[] = { Op::NOOP, Op::DATA, Op::TURN };

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, int dataSize) :
    _program(width, height),
    _data(dataSize),
    _cycleDetector(),
    _dataTracker(_data)
{
    initOpStack(width * height);

    // Init defaults
    _settings.maxSteps = 1024;
    _settings.initialHangSamplePeriod = 16;
    _settings.maxHangDetectAttempts = 4;
    _settings.testHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _opStack;
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

void ExhaustiveSearcher::initOpStack(int size) {
    _opStack = new Op[size];

    for (int i = size; --i >= 0; ) {
        _opStack[i] = Op::UNSET;
    }
}

void ExhaustiveSearcher::dumpOpStack(Op* op) {
    while (*op != Op::UNSET) {
        std::cout << (int)*op << ",";
        op++;
    }
    std::cout << std::endl;
}

void ExhaustiveSearcher::dumpOpStack() {
    std::cout << "Op stack: ";
    dumpOpStack(_opStack);
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

    _data.dumpSettings();
}



bool ExhaustiveSearcher::periodicHangDetected() {
    if (!_data.effectiveDataOperations()) {
        return true;
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
            }
        } else {
            _opsToWaitBeforePeriodicHangCheck = _cyclePeriod;
        }

        _dataTracker.captureSnapShot();
    }

    return false;
}

bool ExhaustiveSearcher::sweepHangDetected() {
    return _dataTracker.sweepHangDetected();
}

void ExhaustiveSearcher::branch(Op* pp, Dir dir, int totalSteps, int depth) {
    Op* pp2 = pp + (int)dir;
    bool resuming = *_resumeFrom != Op::UNSET;

    for (int i = 0; i < 3; i++) {
        Op op = validOps[i];

        if (resuming) {
            if (op == *_resumeFrom) {
                _resumeFrom++;
            } else {
                continue;
            }
        }

        _program.setOp(pp2, op);
        _opStack[depth] = op;
        run(pp, dir, totalSteps, depth + 1);

        if (
            _searchMode == SearchMode::FIND_ONE ||
            (_searchMode == SearchMode::SUB_TREE && resuming)
        ) {
            break;
        }
    }
    _program.clearOp(pp2);
    _opStack[depth] = Op::UNSET;
}

void ExhaustiveSearcher::run(Op* pp, Dir dir, int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    _data.resetHangDetection();
    _dataTracker.reset();
    _cycleDetector.clearInstructionHistory();
    _sampleProgramPointer = nullptr;

    _hangSampleMask = _initialHangSampleMask;
    _remainingPeriodicHangDetectAttempts = _settings.maxHangDetectAttempts;
    _remainingSweepHangDetectAttempts = 0; // Will be bumped after periodic hang checks are done

    while (1) { // Run until branch, termination or error
        Op* pp2;
        bool done = false;
        do { // Execute single step

            pp2 = pp + (int)dir;

            Op op = _program.getOp(pp2);
            switch (op) {
                case Op::DONE:
                    _tracker->reportDone(totalSteps + steps);
                    _data.undo(numDataOps);
                    return;
                case Op::UNSET:
                    branch(pp, dir, totalSteps + steps, depth);
                    _data.undo(numDataOps);
                    return;
                case Op::NOOP:
                    done = true;
                    break;
                case Op::DATA:
                    numDataOps++;
                    switch (dir) {
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
                case Op::TURN:
                    if (_data.val() == 0) {
                        switch (dir) {
                            case Dir::UP: dir = Dir::LEFT; break;
                            case Dir::RIGHT: dir = Dir::UP; break;
                            case Dir::DOWN: dir = Dir::RIGHT; break;
                            case Dir::LEFT: dir = Dir::DOWN; break;
                        }
                    } else {
                        switch (dir) {
                            case Dir::UP: dir = Dir::RIGHT; break;
                            case Dir::RIGHT: dir = Dir::DOWN; break;
                            case Dir::DOWN: dir = Dir::LEFT; break;
                            case Dir::LEFT: dir = Dir::UP; break;
                        }
                    }
                    break;
            }
            if (_remainingPeriodicHangDetectAttempts > 0) {
                _cycleDetector.recordInstruction((char)op | (((char)dir) << 2));
                _opsToWaitBeforePeriodicHangCheck--;
            }
        } while (!done);
        pp = pp2;
        steps++;

//        std::cout << "steps = " << steps << ", depth = " << depth << std::endl;

        if (
            pp == _sampleProgramPointer &&
            dir == _sampleDir &&
            _remainingPeriodicHangDetectAttempts > 0 &&
            _opsToWaitBeforePeriodicHangCheck <= 0
        ) {
//            std::cout << "Back at sample PP: Steps = " << (steps + totalSteps) << std::endl;
//            _cycleDetector.dump();
//            _data.dumpHangInfo();
//            _dataTracker.dump();

            if (periodicHangDetected()) {
                _tracker->reportEarlyHang();
                if (!_settings.testHangDetection) {
                    _data.undo(numDataOps);
                    return;
                }
            }
        }

        if (
            _remainingSweepHangDetectAttempts > 0
        ) {
            DataDirection extensionDir = DataDirection::NONE;
            if (_prevMinBoundP > _data.getMinBoundP()) {
                // Extended leftwards
                extensionDir = DataDirection::LEFT;
            }
            else if (_prevMaxBoundP < _data.getMaxBoundP()) {
                // Extended rightwards
                extensionDir = DataDirection::RIGHT;
            }
            _prevMinBoundP = _data.getMinBoundP();
            _prevMaxBoundP = _data.getMaxBoundP();

            if (extensionDir != DataDirection::NONE && extensionDir != _prevExtensionDir) {
                _prevExtensionDir = extensionDir;
                _extensionCount++;
                if (_extensionCount == 3) {
                    if (sweepHangDetected()) {
//                        std::cout << "Sweep hang detected!" << std::endl;
//                        _data.dump();
//                        _dataTracker.dump();
                        _tracker->reportEarlyHang();
                        if (!_settings.testHangDetection) {
                            _data.undo(numDataOps);
                            return;
                        }
                    }
                    _remainingSweepHangDetectAttempts--;
                    _extensionCount = 1; // Continue with next detection attempt
                }
                _dataTracker.captureSnapShot();
            }
        }

        if (! (steps & _hangSampleMask)) {
            if (steps + totalSteps >= _settings.maxSteps) {
                _tracker->reportHang();
                _data.undo(numDataOps);
                return;
            }

//            std::cout << "Reset Hang Detection: Steps = " << (steps + totalSteps) << std::endl;
//            _tracker->dumpStats();
//            _data.dumpHangInfo();
//            _data.dump();
//            _dataTracker.dump();
//            _cycleDetector.dump();

            if (_remainingPeriodicHangDetectAttempts > 0) {
                _remainingPeriodicHangDetectAttempts--;

                // Initiate new sample (as it may not have been stuck yet)
                _sampleProgramPointer = pp;
                _sampleDir = dir;
                _cyclePeriod = _cycleDetector.getCyclePeriod();
                _opsToWaitBeforePeriodicHangCheck = _cyclePeriod;
//                std::cout << "period = " << _cyclePeriod << std::endl;
                _cycleDetector.clearInstructionHistory();
                _data.resetHangDetection();
                _dataTracker.reset();
                _dataTracker.captureSnapShot();

                _hangSampleMask = (_hangSampleMask << 1) | 1;
            }
            else if (_remainingPeriodicHangDetectAttempts == 0) {
                _remainingPeriodicHangDetectAttempts--;

                _remainingSweepHangDetectAttempts = 3;
                _extensionCount = 0;
                _prevExtensionDir = DataDirection::NONE;
                _prevMinBoundP = _data.getMinBoundP();
                _prevMaxBoundP = _data.getMaxBoundP();
            }
        }
    }
}

void ExhaustiveSearcher::search() {
    _resumeFrom = new Op[1];
    _resumeFrom[0] = Op::UNSET;

    run(_program.startProgramPointer(), Dir::UP, 0, 0);
}

void ExhaustiveSearcher::search(Op* resumeFrom) {
    _resumeFrom = resumeFrom;

    std::cout << "Resuming from: ";
    dumpOpStack(_resumeFrom);

    run(_program.startProgramPointer(), Dir::UP, 0, 0);
}

void ExhaustiveSearcher::searchSubTree(Op* resumeFrom) {
    _searchMode = SearchMode::SUB_TREE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne() {
    _searchMode = SearchMode::FIND_ONE;
    search();
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne(Op* resumeFrom) {
    _searchMode = SearchMode::FIND_ONE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}
