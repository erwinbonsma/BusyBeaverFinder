//
//  ExhaustiveSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <stdint.h>
#include <vector>

#include "Data.h"
#include "ExecutionState.h"
#include "RunSummary.h"

#include "Program.h"

#include "InterpretedProgramBuilder.h"
#include "FastExecutor.h"

#include "ProgressTracker.h"

#include "ExitFinder.h"

class HangDetector;

enum class SearchMode : int8_t {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

struct SearchSettings {
    int maxHangDetectionSteps;
    int maxSteps;
    int undoCapacity;
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

class ExhaustiveSearcher : public ExecutionState {
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

    // When set, hang detection is delayed until search has reached resume point. This is typically
    // used to investigate late escapes
    bool _delayHangDetection;

    // The number of steps when to disable hang detection
    int _hangDetectionEnd;

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

    std::vector<HangDetector*> _hangDetectors;
    ExitFinder _exitFinder;

    ProgressTracker* _tracker;

    void initInstructionStack(int size);

    void reconfigure();
    void initSearch();

    bool executeCurrentBlock();

    // Executes current program (from start) until the maximum number of steps is reached (an
    // assumed hang), or it escapes the current program.
    void fastExecution();

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
    // Implement ExecutionState interface

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

    void searchSubTree(Ins* resumeFrom, bool delayHangDetection = false);

    void findOne();
    void findOne(Ins* resumeFrom);

    void dumpInstructionStack() const;
    bool instructionStackEquals(Ins* reference) const;

    void dumpSettings();
    void dump();
};
