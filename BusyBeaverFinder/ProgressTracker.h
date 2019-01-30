//
//  ProgressTracker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ProgressTracker_h
#define ProgressTracker_h

#include <time.h>
#include <stdio.h>

#include "Program.h"

class ExhaustiveSearcher;

class ProgressTracker {
    int _dumpStatsPeriod = 100000;
    int _dumpStackPeriod = 1000000;

    ExhaustiveSearcher *_searcher = nullptr;

    long _total = 0;
    long _totalSuccess = 0;
    long _totalError = 0;
    long _totalHangs = 0;
    long _totalEarlyErrors = 0;
    long _totalEarlyHangs = 0;
    long _totalFaultyHangs = 0;
    clock_t _startTime;

    bool _earlyHangSignalled = false;

    int _maxStepsSofar = 0;
    Program _bestProgram;

public:
    ProgressTracker(ExhaustiveSearcher *searcher);

    void setDumpStatsPeriod(int val) { _dumpStatsPeriod = val; }
    void setDumpStackPeriod(int val) { _dumpStackPeriod = val; }

    void reportDone(int totalSteps);
    void reportError();
    void reportHang();
    void reportEarlyHang();

    void dumpStats();
    void dumpFinalStats();
};

#endif /* ProgressTracker_h */
