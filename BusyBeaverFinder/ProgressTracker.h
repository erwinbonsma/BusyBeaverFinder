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
    int _dumpBestSofarLimit = 256;
    bool _dumpUndetectedHangs = false;

    ExhaustiveSearcher& _searcher;

    long _total = 0;
    long _totalSuccess = 0;
    long _totalHangsByType[numHangTypes];
    long _totalErrorsByType[numHangTypes];
    long _totalFaultyHangs = 0;
    clock_t _startTime;

#ifdef TRACK_EQUIVALENCE
    ulonglong _equivalenceTotal = 0;
#endif

    HangType _detectedHang = HangType::UNDETECTED;

    int _maxStepsSofar = 0;
    Program _bestProgram;

public:
    ProgressTracker(ExhaustiveSearcher& searcher);

    void setDumpStatsPeriod(int val) { _dumpStatsPeriod = val; }
    void setDumpStackPeriod(int val) { _dumpStackPeriod = val; }
    void setDumpUndetectedHangs(bool flag) { _dumpUndetectedHangs = flag; }
    void setDumpBestSofarLimit(int minSteps) { _dumpBestSofarLimit = minSteps; }

    long getTotalSuccess() { return _totalSuccess; }
    long getTotalErrors();
    long getTotalHangs();
    long getTotalDetectedErrors();
    long getTotalErrors(HangType hangType) { return _totalErrorsByType[(int)hangType]; }
    long getTotalDetectedHangs();
    long getTotalHangs(HangType hangType) { return _totalHangsByType[(int)hangType]; }

    int getMaxStepsFound() { return _maxStepsSofar; }

    void reportDone(int totalSteps);
    void reportError();
    void reportDetectedHang(HangType hangType);
    void reportAssumedHang();

    void dumpStats();
    void dumpHangStats();
    void dumpFinalStats();
};

#endif /* ProgressTracker_h */
