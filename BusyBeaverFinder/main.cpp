//
//  main.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <iostream>

#include "cxxopts.hpp"

#include "Data.h"
#include "Program.h"

int dx[4] = {0, 1, 0, -1};
int dy[4] = {1, 0, -1, 0};

int maxSteps = 1024;
int hangSamplePeriod = 256;
int dumpStatsPeriod = 100000;

Program *program;
Data *data;

Op validOps[] = { Op::NOOP, Op::DATA, Op::TURN };
Op *opStack;

//#define FORCE
#ifdef FORCE
int forceStack[] =
    // #13549 Busy Beaver for 7x7
    { 1,1,1,1,1,2,3,1,1,1,1,1,3,1,3,2,2,3,2,2,1,3,1,3,2,3,3,3,3,3,2,2,3,3,2,1,3,3,3 };
#endif

int total, totalSuccess, totalError, totalHangs, totalEarlyHangs;
clock_t startTime = clock();

int maxStepsSofar = 0;
Program *bestProgram;

void dumpSettings() {
    std::cout
    << "Size = " << program->getWidth() << "x" << program->getHeight()
    << ", DataSize = " << data->getSize()
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
    << ", Total=" << total
    << ", Success=" << totalSuccess
    << ", Errors=" << totalError
    << ", Hangs=" << totalEarlyHangs << "/" << totalHangs
    << ", Time taken=" << (clock() - startTime) / (double)CLOCKS_PER_SEC
    << "\n";
}

void initOpStack(int size) {
    opStack = new Op[size];

    for (int i = size; --i >= 0; ) {
        opStack[i] = Op::UNSET;
    }
}

void dumpOpStack() {
    int i = 0;
    std::cout << "Op stack: ";
    while (opStack[i] != Op::UNSET) {
        std::cout << (int)opStack[i] << ",";
        i++;
    }
    std::cout << "\n";
}

void reportDone(int totalSteps) {
    totalSuccess++;
    if (totalSteps > maxStepsSofar) {
        maxStepsSofar = totalSteps;
        program->clone(*bestProgram);
        if (maxStepsSofar > 256) {
            std::cout << "Best sofar = " << maxStepsSofar << "\n";
            bestProgram->dump();
            data->dump();
            dumpOpStack();
        }
    }
    if (++total % dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void reportError() {
    totalError++;
    if (++total % dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void reportHang(bool early) {
    totalHangs++;
    if (early) {
        totalEarlyHangs++;
    }
    if (++total % dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void run(int x, int y, Dir dir, int totalSteps, int depth);

void branch(int x, int y, Dir dir, int totalSteps, int depth) {
    int _x = x + dx[(int)dir];
    int _y = y + dy[(int)dir];
    for (int i = 0; i < 3; i++) {
        Op op = validOps[i];
#ifdef FORCE
        op = (Op)forceStack[depth];
#endif
        program->setOp(_x, _y, op);
        opStack[depth] = op;
        run(x, y, dir, totalSteps, depth + 1);
    }
    program->clearOp(_x, _y);
}

void run(int x, int y, Dir dir, int totalSteps, int depth) {
    int numDataOps = 0;
    int steps = 0;

    data->resetHangDetection();

    while (1) { // Run until branch, termination or error
        int _x;
        int _y;
        bool done = false;
        do { // Execute single step
            _x = x + dx[(int)dir];
            _y = y + dy[(int)dir];

            if (_x < 0 || _x == program->getWidth() || _y < 0 || _y == program->getHeight()) {
                reportDone(totalSteps + steps);
                data->undo(numDataOps);
                return;
            }

            switch (program->getOp(_x, _y)) {
                case Op::UNSET:
                    branch(x, y, dir, totalSteps + steps, depth);
                    data->undo(numDataOps);
                    return;
                case Op::NOOP:
                    done = true;
                    break;
                case Op::DATA:
                    numDataOps++;
                    switch (dir) {
                        case Dir::UP:
                            data->inc();
                            break;
                        case Dir::DOWN:
                            data->dec();
                            break;
                        case Dir::RIGHT:
                            if (! data->shr()) {
                                reportError();
                                data->undo(numDataOps);
                                return;
                            }
                            break;
                        case Dir::LEFT:
                            if (! data->shl()) {
                                reportError();
                                data->undo(numDataOps);
                                return;
                            }
                            break;
                    }
                    done = true;
                    break;
                case Op::TURN:
                    if (data->val() == 0) {
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

        if (steps == maxSteps) {
            reportHang(false);
            data->undo(numDataOps);
            return;
        }
        else if (steps % hangSamplePeriod == 0) {
            if (data->isHangDetected()) {
                reportHang(true);
                data->undo(numDataOps);
                return;
            }
            data->resetHangDetection();
        }
    }
}

void init(int argc, char * argv[]) {
    cxxopts::Options options("BusyBeaverFinder", "Searcher for Busy Beaver Programs");
    options.add_options()
        ("w,width", "Program width", cxxopts::value<int>())
        ("h,height", "Program height", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("m,max-steps", "Maximum steps to run", cxxopts::value<int>())
        ("p,hang-period", "Period for hang-detection", cxxopts::value<int>())
        ("help", "Show help");
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    int width = 4;
    int height = 4;
    int dataSize = 1024;

    if (result.count("w")) {
        width = result["w"].as<int>();
    }
    if (result.count("h")) {
        height = result["h"].as<int>();
    }
    if (result.count("d")) {
        dataSize = result["d"].as<int>();
    }
    if (result.count("m")) {
        maxSteps = result["m"].as<int>();
    }
    if (result.count("p")) {
        hangSamplePeriod = result["p"].as<int>();
    }

    program = new Program(width, height);
    bestProgram = new Program(width, height);
    data = new Data(dataSize, maxSteps, hangSamplePeriod);

    initOpStack(width * height);
}

int main(int argc, char * argv[]) {
    init(argc, argv);

    dumpSettings();
    run(0, -1, Dir::UP, 0, 0);
    dumpStats();
    bestProgram->dump();

    return 0;
}
