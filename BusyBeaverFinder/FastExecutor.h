//
//  FastExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/05/19.
//  Copyright © 2019 Erwin Bonsma
//
#pragma once

#include "Types.h"

class ProgramBlock;

class FastExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;

    int _maxSteps;
    int _numSteps;

public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    void setMaxSteps(int steps) { _maxSteps = steps; }
    int numSteps() const { return _numSteps; }

    RunResult execute(const ProgramBlock *programBlock);

    void dump();
};
