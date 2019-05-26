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

#include "InterpretedProgram.h"
#include "FastExecutor.h"

#include "DataTracker.h"
#include "ProgressTracker.h"
#include "RunSummary.h"

#include "ExitFinder.h"
#include "PeriodicHangDetector.h"
#include "MetaPeriodicHangDetector.h"
#include "SweepHangDetector.h"
#include "GliderHangDetector.h"

enum class SearchMode : char {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

struct SearchSettings {
    int maxHangDetectionSteps;
    int maxSteps;
    int maxHangDetectAttempts;
    int minWaitBeforeRetryingHangChecks;
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

class ExhaustiveSearcher {
    SearchSettings _settings;

    Program _program;
    Data _data;
    DataTracker _dataTracker;

    // Nested run summaries. The first summarizes the program execution, identifying loops along the
    // way. The second summarizes the first run summary. In particular, it signals repeated patterns
    // in the first summary.
    RunSummary _runSummary[2];

    // Helper buffer to store temporary Z-Array that is needed by some utility functions
    int* _zArrayHelperBuf;

    // Determines when to abort the search
    SearchMode _searchMode;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Ins* _resumeFrom;

    ProgramPointer _pp;
    ProgramBlock* _block;
    int _numSteps;

    // Stack of instructions built up by the exhaustive search
    Ins* _instructionStack;

    // An interpreted representation of the program
    InterpretedProgram _interpretedProgram;

    FastExecutor _fastExecutor;

    int _numHangDetectAttempts;

    PeriodicHangDetector* _periodicHangDetector;
    MetaPeriodicHangDetector* _metaPeriodicHangDetector;
    SweepHangDetector* _sweepHangDetector;
    GliderHangDetector* _gliderHangDetector;
    ExitFinder _exitFinder;
    int _waitBeforeRetryingHangChecks;

    ProgressTracker* _tracker;

    void initInstructionStack(int size);

    HangDetector* initiateNewHangCheck();
    bool periodicHangDetected();
    bool sweepHangDetected();
    bool isSweepDiverging();

    void reconfigure();

    bool executeCurrentBlock();

    ProgramPointer executeCompiledBlocksWithBacktracking();
    ProgramPointer executeCompiledBlocksWithHangDetection();
    ProgramPointer executeCompiledBlocks();

    void run(int depth);
    void branch(int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);
    ~ExhaustiveSearcher();

    SearchSettings getSettings() { return _settings; }

    // Updates settings. It changes buffer allocations as needed.
    void configure(SearchSettings settings);

    bool getHangDetectionTestMode() { return _settings.testHangDetection; }

    ProgressTracker* getProgressTracker() { return _tracker; }
    void setProgressTracker(ProgressTracker* tracker);

    Program& getProgram() { return _program; }
    Data& getData() { return _data; }
    InterpretedProgram& getInterpretedProgram() { return _interpretedProgram; }

    DataTracker& getDataTracker() { return _dataTracker; }
    RunSummary& getRunSummary() { return _runSummary[0]; }
    RunSummary& getMetaRunSummary() { return _runSummary[1]; }

    int getNumSteps() { return _numSteps; }
    int getNumHangDetectAttempts() { return _numHangDetectAttempts; }

    ProgramBlock* getProgramBlock() { return _block; }

    bool atTargetProgram();

    void search();
    void search(Ins* resumeFrom);

    void searchSubTree(Ins* resumeFrom);

    void findOne();
    void findOne(Ins* resumeFrom);

    void dumpInstructionStack();
    bool instructionStackEquals(Ins* reference);

    void dumpSettings();
    void dumpHangDetection();
    void dump();
};

#endif /* ExhaustiveSearcher_h */
