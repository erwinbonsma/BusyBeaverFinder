//
//  FastExecutor.h
//  BusyBeaverFinder
//
//  Created by Erwin on 22/05/19.
//  Copyright © 2019 Erwin Bonsma
//
#pragma once

#include "ProgramExecutor.h"
#include "Types.h"
#include "Data.h"

class FastExecutor : public ProgramExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;

    bool _canResume;

    RunResult run();

public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    void pop() override { _canResume = false; };

    RunResult execute(const InterpretedProgram* program) override;

    void resumeFrom(const ProgramBlock* resumeFrom, const Data& data, int numSteps);

    HangType detectedHangType() const override { return HangType::NO_DATA_LOOP; }

    void dump() const override;
};
