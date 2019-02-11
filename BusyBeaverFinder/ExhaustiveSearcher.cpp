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
    _program(width, height), _data(dataSize), _cycleDetector(), _dataTracker(_data)
{
    initOpStack(width * height);

    // Init defaults
    setMaxStepsTotal(1024);
    setMaxStepsPerRun(1024);
    setHangSamplePeriod(64);
    setMaxHangDetectAttempts(4);
    setHangDetectionTestMode(false);
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _opStack;
}


void ExhaustiveSearcher::setMaxStepsTotal(int val) {
    _maxStepsTotal = val;
    _data.setStackSize(val);
}

void ExhaustiveSearcher::setMaxStepsPerRun(int val) {
    _maxStepsPerRun = val;
}

void ExhaustiveSearcher::setHangSamplePeriod(int val) {
    assert(isPowerOfTwo(val));
    _hangSamplePeriod = val;
    _hangSampleMask = val - 1;
    _data.setHangSamplePeriod(val);
    _cycleDetector.setHangSamplePeriod(val);
}

void ExhaustiveSearcher::setMaxHangDetectAttempts(int val) {
    _maxHangDetectAttempts = val;
}

void ExhaustiveSearcher::setHangDetectionTestMode(bool val) {
    _testHangDetection = val;
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
    << ", MaxSteps = " << _maxStepsPerRun << "/" << _maxStepsTotal
    << ", HangSamplePeriod = " << _hangSamplePeriod
    << ", MaxDetectAttempts = " << _maxHangDetectAttempts
    << ", TestHangDetection = " << _testHangDetection
    << std::endl;

    _data.dumpSettings();
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
}

bool ExhaustiveSearcher::earlyHangDetected() {
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
            if (_dataTracker.compareSnapShotDeltas()) {
                return true;
            }
        } else {
            _opsToWaitBeforeHangCheck = _cyclePeriod;
        }

        _dataTracker.captureSnapShot();
        // TODO: Abort after X failed attempts?
    }

    return false;
}

void ExhaustiveSearcher::run(Op* pp, Dir dir, int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    _data.resetHangDetection();
    _dataTracker.reset();
    _cycleDetector.clearInstructionHistory();
    _sampleProgramPointer = nullptr;
    _remainingHangDetectAttempts = _maxHangDetectAttempts;

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
            if (_remainingHangDetectAttempts > 0) {
                _cycleDetector.recordInstruction(op);
                _opsToWaitBeforeHangCheck--;
            }
        } while (!done);
        pp = pp2;
        steps++;

//        std::cout << "steps = " << steps << "depth = " << depth << std::endl;

        if (
            pp == _sampleProgramPointer &&
            dir == _sampleDir &&
            _remainingHangDetectAttempts > 0 &&
            _opsToWaitBeforeHangCheck <= 0
        ) {
//            std::cout << "Back at sample PP: Steps = " << (steps + totalSteps) << std::endl;
//            _cycleDetector.dump();
//            _data.dumpHangInfo();
//            _dataTracker.dump();

            if (earlyHangDetected()) {
                _tracker->reportEarlyHang();
                if (!_testHangDetection) {
                    _data.undo(numDataOps);
                    return;
                }
            }
        }

        if (! (steps & _hangSampleMask)) {
            if (
                (steps + totalSteps + _hangSamplePeriod >= _maxStepsTotal) ||
                (steps == _maxStepsPerRun)
            ) {
                _tracker->reportHang();
                _data.undo(numDataOps);
                return;
            }

//            std::cout << "Reset Hang Detection: Steps = " << (steps + totalSteps) << std::endl;
//            _tracker->dumpStats();
//            _data.dumpHangInfo();
//            _data.dump();
//            _program.dumpHangInfo();

            if (_remainingHangDetectAttempts-- > 0) {
                // Initiate new sample (as it may not have been stuck yet)
                _sampleProgramPointer = pp;
                _sampleDir = dir;
                _cyclePeriod = _cycleDetector.getCyclePeriod();
                _opsToWaitBeforeHangCheck = _cyclePeriod;
//                std::cout << "period = " << _cyclePeriod << std::endl;
                _cycleDetector.clearInstructionHistory();
                _data.resetHangDetection();
                _dataTracker.reset();
                _dataTracker.captureSnapShot();
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
