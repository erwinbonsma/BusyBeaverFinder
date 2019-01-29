//
//  ExhaustiveSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include "ExhaustiveSearcher.h"

#include <iostream>
#include <assert.h>

#include "Utils.h"

Op validOps[] = { Op::NOOP, Op::DATA, Op::TURN };

const int dx[4] = {0, 1, 0, -1};
const int dy[4] = {1, 0, -1, 0};

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, int dataSize) :
    _program(width, height), _data(dataSize)
{
    initOpStack(width * height);

    // Init defaults
    setMaxStepsTotal(1024);
    setMaxStepsPerRun(1024);
    setHangSamplePeriod(64);
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

void ExhaustiveSearcher::initOpStack(int size) {
    _opStack = new Op[size];

    for (int i = size; --i >= 0; ) {
        _opStack[i] = Op::UNSET;
    }
}

void ExhaustiveSearcher::dumpOpStack() {
    int i = 0;
    std::cout << "Op stack: ";
    while (_opStack[i] != Op::UNSET) {
        std::cout << (int)_opStack[i] << ",";
        i++;
    }
    std::cout << std::endl;
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _data.getSize()
    << ", MaxSteps = " << _maxStepsPerRun << "/" << _maxStepsTotal
    << ", HangSamplePeriod = " << _hangSamplePeriod
    << std::endl;

    _data.dumpSettings();

#ifdef FORCE
    std::cout << "FORCE ACTIVE" << std::endl;
#endif
#ifdef RESUME
    std::cout << "RESUME ACTIVE" << std::endl;
#endif
}


void ExhaustiveSearcher::branch(int x, int y, Dir dir, int totalSteps, int depth) {
    int x2 = x + dx[(int)dir];
    int y2 = y + dy[(int)dir];
    for (int i = 0; i < 3; i++) {
        Op op = validOps[i];
#ifdef FORCE
        op = (Op)forceStack[depth];
#endif
#ifdef RESUME
        op = (Op)validOps[(i + resumeStack[depth] - 1) % 3];
#endif
        _program.setOp(x2, y2, op);
        _opStack[depth] = op;
        run(x, y, dir, totalSteps, depth + 1);
    }
    _program.clearOp(x2, y2);
}

void ExhaustiveSearcher::run(int x, int y, Dir dir, int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    _data.resetHangDetection();

    while (1) { // Run until branch, termination or error
        int x2;
        int y2;
        bool done = false;
        do { // Execute single step

            // Unwrap position update to only update and check coordinate that changed
            if ((char)dir & 1) {
                if (dir == Dir::LEFT) {
                    x2 = x - 1;
                    done = x2 < 0;
                } else {
                    x2 = x + 1;
                    done = x2 == _program.getWidth();
                }
                y2 = y;
            } else {
                if (dir == Dir::DOWN) {
                    y2 = y - 1;
                    done = y2 < 0;
                } else {
                    y2 = y + 1;
                    done = y2 == _program.getHeight();
                }
                x2 = x;
            }

            if (done) {
                _tracker->reportDone(totalSteps + steps);
                _data.undo(numDataOps);
                return;
            } else {
                switch (_program.getOp(x2, y2)) {
                    case Op::UNSET:
                        branch(x, y, dir, totalSteps + steps, depth);
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
                            dir = (Dir) (((int)dir + 3) % 4);
                        } else {
                            dir = (Dir) (((int)dir + 1) % 4);
                        }
                        break;
                }
            }
        } while (!done);
        x = x2;
        y = y2;
        steps++;

        if (! (steps & _hangSampleMask)) {
            if (
                (steps + totalSteps + _hangSamplePeriod >= _maxStepsTotal) ||
                (steps == _maxStepsPerRun)
            ) {
                _tracker->reportHang(false);
                _data.undo(numDataOps);
                return;
            }

            if (_data.isHangDetected()) {
                _tracker->reportHang(true);
                _data.undo(numDataOps);
                return;
            }
            _data.resetHangDetection();
        }
    }
}

void ExhaustiveSearcher::search() {
    run(0, -1, Dir::UP, 0, 0);
}
