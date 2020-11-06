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
class HangDetector;

class ProgressTracker {
    int _dumpStatsPeriod = 100000;
    int _dumpStackPeriod = 1000000;
    int _dumpBestSofarLimit = 256;
    bool _dumpUndetectedHangs = false;

    ExhaustiveSearcher& _searcher;

    long _total = 0;
    long _totalSuccess = 0;
    long _totalFastExecutions = 0;
    long _totalLateEscapes = 0;
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

    // Hang detector with details of the last detected specialized hang (i.e. hang that was
    // detected by a HangDetector).
    const HangDetector *_lastDetectedHang;

    // Stats on hang-detection speed and effectiveness
    int _maxStepsUntilHangDetection = 0;

    void report();

public:
    ProgressTracker(ExhaustiveSearcher& searcher);

    void setDumpStatsPeriod(int val) { _dumpStatsPeriod = val; }
    void setDumpStackPeriod(int val) { _dumpStackPeriod = val; }
    void setDumpUndetectedHangs(bool flag) { _dumpUndetectedHangs = flag; }
    void setDumpBestSofarLimit(int minSteps) { _dumpBestSofarLimit = minSteps; }

    long getTotalSuccess() const { return _totalSuccess; }
    long getTotalErrors() const;
    long getTotalHangs() const;
    long getTotalFastExecutions() const { return _totalFastExecutions; }
    long getTotalLateEscapes() const { return _totalLateEscapes; }
    long getTotalDetectedErrors() const;
    long getTotalErrors(HangType hangType) const { return _totalErrorsByType[(int)hangType]; }
    long getTotalDetectedHangs() const;
    long getTotalHangs(HangType hangType) const { return _totalHangsByType[(int)hangType]; }

    const HangDetector* getLastDetectedHang() const { return _lastDetectedHang; }

    int getMaxStepsFound() const { return _maxStepsSofar; }

    void reportDone(int totalSteps);
    void reportError();
    void reportDetectedHang(HangType hangType);
    void reportDetectedHang(const HangDetector* hangDetector);
    void reportAssumedHang();

    void reportFastExecution() { _totalFastExecutions++; }
    // A "late escape" is a program that did not terminate while hang detection was enabled, but
    // whose execution escaped from its interpreted program during fast execution (at which time
    // the program cannot be expanded further).
    void reportLateEscape(int numSteps);

    void dumpStats();
    void dumpHangStats();
    void dumpFinalStats();
};

#endif /* ProgressTracker_h */
