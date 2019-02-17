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

enum class SearchMode : char {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

enum class DataDirection : char {
    NONE = 0,
    LEFT = 1,
    RIGHT = 2
};

enum class HangCheck : char {
    NONE = 0,
    PERIODIC = 1,
    SWEEP = 2
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

    int _hangSampleMask;
    HangCheck _activeHangCheck;

    // Periodic hang detection
    Op* _sampleProgramPointer;
    Dir _sampleDir;
    int _cyclePeriod;
    int _opsToWaitBeforePeriodicHangCheck;
    int _remainingPeriodicHangDetectAttempts;

    // Sweep hang detection
    int _remainingSweepHangDetectAttempts;
    int *_prevMinBoundP, *_prevMaxBoundP;
    DataDirection _prevExtensionDir;
    int _extensionCount;

    ProgressTracker* _tracker;

    void dumpOpStack(Op* op);
    void initOpStack(int size);

    void initiateNewHangCheck(Op* pp, Dir dir);
    bool periodicHangDetected();
    bool sweepHangDetected();

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
