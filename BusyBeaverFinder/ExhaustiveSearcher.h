//
//  ExhaustiveSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ExhaustiveSearcher_h
#define ExhaustiveSearcher_h

#include <stdio.h>

#include "Data.h"
#include "Program.h"
#include "ProgressTracker.h"

class ExhaustiveSearcher {
    int _hangSamplePeriod;
    int _hangSampleMask;
    int _maxStepsPerRun;
    int _maxStepsTotal;

    Program _program;
    Data _data;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Op* _resumeFrom;

    // Stack of operations built up by the exhaustive search
    Op* _opStack;

    ProgressTracker* _tracker;

    void initOpStack(int size);

    void run(int x, int y, Dir dir, int totalSteps, int depth);
    void branch(int x, int y, Dir dir, int totalSteps, int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);

    int getHangSamplePeriod() { return _hangSamplePeriod; }
    int getMaxStepsPerRun() { return _maxStepsPerRun; }
    int getMaxStepsTotal() { return _maxStepsTotal; }

    void setMaxStepsTotal(int val);
    void setMaxStepsPerRun(int val);
    void setHangSamplePeriod(int val);

    void setProgressTracker(ProgressTracker* tracker) { _tracker = tracker; }

    Program& getProgram() { return _program; }
    Data& getData() { return _data; }

    void search();
    void search(Op* resumeFrom);

    void dumpOpStack();
    void dumpSettings();
};

#endif /* ExhaustiveSearcher_h */
