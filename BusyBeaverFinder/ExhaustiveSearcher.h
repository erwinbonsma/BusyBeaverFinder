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
    bool _cycleDetectorEnabled;
    CycleDetector _cycleDetector;
    DataTracker _dataTracker;

    // Determines when to abort the search
    SearchMode _searchMode;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Ins* _resumeFrom;

    ProgramPointer _pp;

    // Stack of instructions built up by the exhaustive search
    Ins* _instructionStack;

    // Recent turn status, which is generally useful for hang detection
    bool _performedTurn;
    bool _lastTurnWasRight;

    int _hangSampleMask;
    HangCheck _activeHangCheck;

    // Periodic hang detection
    ProgramPointer _samplePp;
    int _cyclePeriod;
    // When to perform the periodic hang check (in number of recorded instructions)
    int _periodicHangCheckAt;
    int _remainingPeriodicHangDetectAttempts;

    // Sweep hang detection
    int _remainingSweepHangDetectAttempts;
    DataDirection _prevExtensionDir;
    int _extensionCount;
    int* _sweepMidTurningPoint;
    DataDirection _sweepMidTurningDir;
    ProgramPointer _sweepStartPp;
    bool _midSequence;

    ProgressTracker* _tracker;

    void dumpInstructionStack(Ins* stack);
    void initInstructionStack(int size);

    void initiateNewHangCheck();
    bool periodicHangDetected();
    bool sweepHangDetected();
    bool isSweepDiverging();

    void reconfigure();

    void run(int totalSteps, int depth);
    void branch(int totalSteps, int depth);
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
    void search(Ins* resumeFrom);

    void searchSubTree(Ins* resumeFrom);

    void findOne();
    void findOne(Ins* resumeFrom);

    void dumpInstructionStack();
    void dumpSettings();
};

#endif /* ExhaustiveSearcher_h */
