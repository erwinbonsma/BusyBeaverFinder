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

class FastExecutor : public ProgramExecutor {
    int* _data;
    int _dataBufSize;

    int* _minDataP;
    int* _midDataP;
    int* _maxDataP;

    int* _dataP;

public:
    FastExecutor(int dataSize);
    ~FastExecutor();

    RunResult execute(const InterpretedProgram* program) override;
    HangType detectedHangType() const override { return HangType::NO_DATA_LOOP; }

    void dump() const override;
};
