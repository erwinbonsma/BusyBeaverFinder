//
//  FastExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/05/19.
//  Copyright © 2019 Erwin Bonsma
//
#pragma once

#include "ProgramExecutor.h"

class FastExecutor : public ProgramExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;
    int _numSteps;

public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    RunResult execute(const ProgramBlock *programBlock, int maxSteps) override;
    int numSteps() const override { return _numSteps; }

    void dump();
};
