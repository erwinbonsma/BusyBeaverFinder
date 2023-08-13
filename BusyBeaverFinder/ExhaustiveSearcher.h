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

#include "Program.h"

#include "InterpretedProgramBuilder.h"
#include "FastExecutor.h"
#include "HangExecutor.h"

#include "ProgressTracker.h"

#include "ExitFinder.h"

enum class SearchMode : int8_t {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

struct SearchSettings {
    int dataSize;
    int maxHangDetectionSteps;
    int maxSteps;
    int undoCapacity;
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

SearchSettings defaultSettings() {
    return SearchSettings {
        .maxSteps = 1024,
        .maxHangDetectionSteps = 1024,
        .undoCapacity = 1024,
        .testHangDetection = false,
        .disableNoExitHangDetection = false
    };
}

class ExhaustiveSearcher {
    SearchSettings _settings;

    Program _program;

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

//    ProgramPointer _pp;
//    ProgramBlock* _block;
//    int _numSteps;

    // Stack of instructions built up by the exhaustive search
    Ins* _instructionStack;

    // An interpreted representation of the program
    InterpretedProgramBuilder _programBuilder;

    FastExecutor _fastExecutor;
    HangExecutor _hangExecutor;

    ExitFinder _exitFinder;

    ProgressTracker* _tracker;

    void initInstructionStack(int size);

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
    ExhaustiveSearcher(int width, int height, SearchSettings settings);
    ~ExhaustiveSearcher();

    SearchSettings getSettings() { return _settings; }

    bool getHangDetectionTestMode() { return _settings.testHangDetection; }

    ProgressTracker* getProgressTracker() { return _tracker; }
    void setProgressTracker(ProgressTracker* tracker);

    const Program& getProgram() const { return _program; }

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
