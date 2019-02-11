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

#include "CycleDetector.h"
#include "DataTracker.h"
#include "ProgressTracker.h"

enum SearchMode : char {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

class ExhaustiveSearcher {
    int _hangSamplePeriod;
    int _hangSampleMask;
    int _maxStepsPerRun;
    int _maxStepsTotal;
    int _maxHangDetectAttempts;
    bool _testHangDetection;

    Program _program;
    Data _data;
    CycleDetector _cycleDetector;
    DataTracker _dataTracker;

    // Determines when to abort the search
    SearchMode _searchMode;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Op* _resumeFrom;

    // Stack of operations built up by the exhaustive search
    Op* _opStack;

    // Hang detection
    Op* _sampleProgramPointer;
    Dir _sampleDir;
    int _cyclePeriod;
    int _opsToWaitBeforeHangCheck;

    int _remainingHangDetectAttempts;

    ProgressTracker* _tracker;

    void dumpOpStack(Op* op);
    void initOpStack(int size);

    bool earlyHangDetected();

    void run(Op* pp, Dir dir, int totalSteps, int depth);
    void branch(Op* pp, Dir dir, int totalSteps, int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);
    ~ExhaustiveSearcher();

    int getHangSamplePeriod() { return _hangSamplePeriod; }
    int getMaxStepsPerRun() { return _maxStepsPerRun; }
    int getMaxStepsTotal() { return _maxStepsTotal; }
    int getMaxHangDetectAttempts() { return _maxHangDetectAttempts; }
    bool getHangDetectionTestMode() { return _testHangDetection; }

    void setMaxStepsTotal(int val);
    void setMaxStepsPerRun(int val);
    void setHangSamplePeriod(int val);
    void setMaxHangDetectAttempts(int val);
    void setHangDetectionTestMode(bool val);

    void setProgressTracker(ProgressTracker* tracker) { _tracker = tracker; }

    Program& getProgram() { return _program; }
    Data& getData() { return _data; }

    void search();
    void search(Op* resumeFrom);

    void searchSubTree(Op* resumeFrom);

    void findOne();
    void findOne(Op* resumeFrom);

    void dumpOpStack();
    void dumpSettings();
};

#endif /* ExhaustiveSearcher_h */
