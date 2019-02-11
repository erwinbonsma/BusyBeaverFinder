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

struct SearchSettings {
    int initialHangSamplePeriod;
    int maxSteps;
    int maxHangDetectAttempts;
    bool testHangDetection;
};

class ExhaustiveSearcher {
    SearchSettings _settings;
    // Derived settings
    int _initialHangSampleMask;

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

    int _hangSampleMask;
    int _remainingHangDetectAttempts;

    ProgressTracker* _tracker;

    void dumpOpStack(Op* op);
    void initOpStack(int size);

    bool earlyHangDetected();

    void reconfigure();

    void run(Op* pp, Dir dir, int totalSteps, int depth);
    void branch(Op* pp, Dir dir, int totalSteps, int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);
    ~ExhaustiveSearcher();


    SearchSettings getSettings() { return _settings; }

    // Updates settings. It changes buffer allocations as needed.
    void configure(SearchSettings settings);

    bool getHangDetectionTestMode() { return _settings.testHangDetection; }

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
