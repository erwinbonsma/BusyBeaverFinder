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
#include "ProgressTracker.h"

class ExhaustiveSearcher {
    int _hangSamplePeriod;
    int _hangSampleMask;
    int _maxStepsPerRun;
    int _maxStepsTotal;
    bool _testHangDetection;

    Program _program;
    Data _data;
    CycleDetector _cycleDetector;

    // When set to true, aborts the search. Note, the current program that is being built/evaluated
    // will end first. This is exploited by the findOne methods.
    bool _abortSearch = false;

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

    ProgressTracker* _tracker;

    void dumpOpStack(Op* op);
    void initOpStack(int size);

    void run(Op* pp, Dir dir, int totalSteps, int depth);
    void branch(Op* pp, Dir dir, int totalSteps, int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);
    ~ExhaustiveSearcher();

    int getHangSamplePeriod() { return _hangSamplePeriod; }
    int getMaxStepsPerRun() { return _maxStepsPerRun; }
    int getMaxStepsTotal() { return _maxStepsTotal; }
    bool getHangDetectionTestMode() { return _testHangDetection; }

    void setMaxStepsTotal(int val);
    void setMaxStepsPerRun(int val);
    void setHangSamplePeriod(int val);
    void setHangDetectionTestMode(bool val);

    void setProgressTracker(ProgressTracker* tracker) { _tracker = tracker; }

    Program& getProgram() { return _program; }
    Data& getData() { return _data; }

    void search();
    void search(Op* resumeFrom);

    void findOne();
    void findOne(Op* resumeFrom);

    void dumpOpStack();
    void dumpSettings();
};

#endif /* ExhaustiveSearcher_h */
