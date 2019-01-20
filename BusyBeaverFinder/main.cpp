//
//  main.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <iostream>

#include "Data.h"
#include "Program.h"

int dx[4] = {0, 1, 0, -1};
int dy[4] = {1, 0, -1, 0};

Program program;
Data data;

Op validOps[] = { Op::NOOP, Op::DATA, Op::TURN };

int totalDone, totalError, totalHangs, totalEarlyHangs;
clock_t startTime = clock();

int maxStepsSofar = 0;
Program bestProgram;

void dumpSettings() {
    std::cout
    << "Size = " << w << "x" << h
    << ", DataSize = " << dataSize
    << ", MaxSteps = " << maxSteps
    << ", HangSamplePeriod = " << hangSamplePeriod
    << "\n";

    std::cout
    << "Enabled hang detections:"
#ifdef HANG_DETECTION1
    << " 1"
#endif
#ifdef HANG_DETECTION2
    << " 2"
#endif
    << "\n";
}

void dumpStats() {
    std::cout
    << "Best=" << maxStepsSofar
    << ", Done=" << totalDone
    << ", Errors=" << totalError
    << ", Hangs=" << totalEarlyHangs << "/" << totalHangs
    << ", Time taken=" << (clock() - startTime) / (double)CLOCKS_PER_SEC
    << "\n";
}

void reportDone(int totalSteps) {
    totalDone++;
    if (totalSteps > maxStepsSofar) {
        maxStepsSofar = totalSteps;
        program.clone(bestProgram);
        if (maxStepsSofar > 256) {
            std::cout << "Best sofar = " << maxStepsSofar << "\n";
            bestProgram.dump();
            data.dump();
        }
    }
    if (totalDone % 100000 == 0) {
        dumpStats();
    }
}

void reportError() {
    totalError++;
}

void reportHang(bool early) {
    totalHangs++;
    if (early) {
        totalEarlyHangs++;
    }
}

void run(int x, int y, Dir dir, int totalSteps);

void branch(int x, int y, Dir dir, int totalSteps) {
    int _x = x + dx[(int)dir];
    int _y = y + dy[(int)dir];
    for (int i = 0; i < 3; i++) {
        program.setOp(_x, _y, validOps[i]);
        run(x, y, dir, totalSteps);
    }
    program.clearOp(_x, _y);
}

void run(int x, int y, Dir dir, int totalSteps) {
    int numDataOps = 0;
    int steps = 0;

    data.resetHangDetection();

    while (1) { // Run until branch, termination or error
        int _x;
        int _y;
        bool done = false;
        do { // Execute single step
            _x = x + dx[(int)dir];
            _y = y + dy[(int)dir];

            if (_x < 0 || _x == w || _y < 0 || _y == h) {
                reportDone(totalSteps + steps);
                data.undo(numDataOps);
                return;
            }

            switch (program.getOp(_x, _y)) {
                case Op::UNSET:
                    branch(x, y, dir, totalSteps + steps);
                    data.undo(numDataOps);
                    return;
                case Op::NOOP:
                    done = true;
                    break;
                case Op::DATA:
                    numDataOps++;
                    switch (dir) {
                        case Dir::UP:
                            data.inc();
                            break;
                        case Dir::DOWN:
                            data.dec();
                            break;
                        case Dir::RIGHT:
                            if (! data.shr()) {
                                reportError();
                                data.undo(numDataOps);
                                return;
                            }
                            break;
                        case Dir::LEFT:
                            if (! data.shl()) {
                                reportError();
                                data.undo(numDataOps);
                                return;
                            }
                            break;
                    }
                    done = true;
                    break;
                case Op::TURN:
                    if (data.val() == 0) {
                        dir = (Dir) (((int)dir + 3) % 4);
                    } else {
                        dir = (Dir) (((int)dir + 1) % 4);
                    }
                    break;
            }
        } while (!done);
        x = _x;
        y = _y;
        steps++;

        if (steps > maxSteps) {
            reportHang(false);
            data.undo(numDataOps);
            return;
        }
        else if (steps % hangSamplePeriod == 0) {
            if (data.isHangDetected()) {
                reportHang(true);
                data.undo(numDataOps);
                return;
            }
            data.resetHangDetection();
        }
    }
}

int main(int argc, const char * argv[]) {
    dumpSettings();
    run(0, -1, Dir::UP, 0);
    dumpStats();
    bestProgram.dump();

    std::cout
        << "Time taken: "
        << (clock() - startTime) / (double)CLOCKS_PER_SEC << "\n";

    return 0;
}
