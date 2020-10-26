//
//  ExhaustiveSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ExhaustiveSearcher_h
#define ExhaustiveSearcher_h

#include "ProgramExecutor.h"

#include "Program.h"

#include "InterpretedProgramBuilder.h"
#include "FastExecutor.h"

#include "ProgressTracker.h"

#include "ExitFinder.h"

class HangDetector;

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
    int undoCapacity;
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

class ExhaustiveSearcher : public ProgramExecutor {
    SearchSettings _settings;

    Program _program;
    Data _data;

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
    InterpretedProgramBuilder _interpretedProgramBuilder;

    FastExecutor _fastExecutor;

    HangDetector* _hangDetectors[4];
    ExitFinder _exitFinder;

    ProgressTracker* _tracker;

    void initInstructionStack(int size);

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

    const Program& getProgram() const { return _program; }

    //----------------------------------------------------------------------------------------------
    // Implement ProgramExecutor interface

    const InterpretedProgram& getInterpretedProgram() const override {
        return _interpretedProgramBuilder;
    }

    const Data& getData() const override { return _data; }

    const RunSummary& getRunSummary() const override { return _runSummary[0]; }
    const RunSummary& getMetaRunSummary() const override { return _runSummary[1]; }

    void dumpExecutionState() const override;

    //----------------------------------------------------------------------------------------------

    int getNumSteps() { return _numSteps; }

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
    void dump();
};

#endif /* ExhaustiveSearcher_h */
