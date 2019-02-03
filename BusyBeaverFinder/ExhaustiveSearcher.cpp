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
    _program(width, height), _data(dataSize)
{
    initOpStack(width * height);

    // Init defaults
    setMaxStepsTotal(1024);
    setMaxStepsPerRun(1024);
    setHangSamplePeriod(64);
    setHangDetectionTestMode(false);
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
    << ", TestHangDetection = " << _testHangDetection
    << std::endl;

    _data.dumpSettings();
}


void ExhaustiveSearcher::branch(Op* pp, Dir dir, int totalSteps, int depth) {
    Op* pp2 = pp + (int)dir;
    for (int i = 0; i < 3; i++) {
        Op op = validOps[i];

        if (*_resumeFrom != Op::UNSET) {
            if (op == *_resumeFrom) {
                _resumeFrom++;
            } else {
                continue;
            }
        }

        _program.setOp(pp2, op);
        _opStack[depth] = op;
        run(pp, dir, totalSteps, depth + 1);

        if (_abortSearch) {
            break;
        }
    }
    _program.clearOp(pp2);
}

void ExhaustiveSearcher::run(Op* pp, Dir dir, int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    _data.resetHangDetection();
    _program.resetHangDetection();
#ifdef HANG_DETECTION3
    _sampleProgramPointer = nullptr;
#endif

    while (1) { // Run until branch, termination or error
        Op* pp2;
        bool done = false;
        do { // Execute single step

            pp2 = pp + (int)dir;

            switch (_program.getOp(pp2)) {
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
        } while (!done);
        pp = pp2;
        steps++;

#ifdef HANG_DETECTION3
        if (
            pp == _sampleProgramPointer &&
            dir == _sampleDir
        ) {
            bool hangDetected = false;

            if (!_data.significantDataChanges()) {
                hangDetected = true;
            }

            if (!hangDetected) {
                if (_data.getDataPointer() == _sampleDataPointer) {
                    SnapShotComparison result = _data.compareToSnapShot();
                    if (result != SnapShotComparison::IMPACTFUL) {
                        hangDetected = true;
                    }
                }
            }

            if (hangDetected) {
                _tracker->reportEarlyHang();
                if (!_testHangDetection) {
                    _data.undo(numDataOps);
                    return;
                }
            }
            _sampleProgramPointer = nullptr;
        }
#endif

        if (! (steps & _hangSampleMask)) {
            if (
                (steps + totalSteps + _hangSamplePeriod >= _maxStepsTotal) ||
                (steps == _maxStepsPerRun)
            ) {
                _tracker->reportHang();
                _data.undo(numDataOps);
                return;
            }

//            std::cout << "Steps = " << (steps + totalSteps) << std::endl;
//            _tracker->dumpStats();
//            _data.dumpHangInfo();
//            _data.dump();
//            _program.dumpHangInfo();

            // Initiate new sample (as it may not have been stuck yet)
            _sampleProgramPointer = pp;
            _sampleDataPointer = _data.getDataPointer();
            _sampleDir = dir;
            _data.captureSnapShot();
            _data.resetHangDetection();
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

void ExhaustiveSearcher::findOne() {
    _abortSearch = true;
    search();
    _abortSearch = false;
}

void ExhaustiveSearcher::findOne(Op* resumeFrom) {
    _abortSearch = true;
    search(resumeFrom);
    _abortSearch = false;
}
