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

const SearchSettings defaultSearchSettings = {
    .dataSize = 1024,
    .maxSteps = 1024,
    .maxHangDetectionSteps = 1024,
    .undoCapacity = 1024,
    .testHangDetection = false,
    .disableNoExitHangDetection = false
};

class ExhaustiveSearcher {
    SearchSettings _settings;

    Program _program;

    // Determines when to abort the search
    SearchMode _searchMode;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Ins* _resumeFrom;

    TurnDirection _td;
    ProgramPointer _pp;
    int _hangDetectionStart;

    // Stack of instructions built up by the exhaustive search
    std::vector<Ins> _instructionStack;

    // An interpreted representation of the program
    InterpretedProgramBuilder _programBuilder;

    FastExecutor _fastExecutor;
    HangExecutor _hangExecutor;
    // Points to the active executor
    ProgramExecutor* _programExecutor;

    ExitFinder _exitFinder;

    ProgressTracker* _tracker;

    void run();
    void branch();
    void extendBlock();
    void buildBlock(const ProgramBlock* block);
public:
    ExhaustiveSearcher(int width, int height, SearchSettings settings);

    SearchSettings getSettings() { return _settings; }

    bool getHangDetectionTestMode() { return _settings.testHangDetection; }

    ProgressTracker* getProgressTracker() { return _tracker; }
    void setProgressTracker(ProgressTracker* tracker);

    const Program& getProgram() const { return _program; }
    const InterpretedProgram& getInterpretedProgram() const { return _programBuilder; }

    const ProgramExecutor* getProgramExecutor() const { return _programExecutor; }

    //----------------------------------------------------------------------------------------------

    bool atTargetProgram();

    void search();
    void search(Ins* resumeFrom);

    void searchSubTree(Ins* resumeFrom);

    void findOne();
    void findOne(Ins* resumeFrom);

    void dumpInstructionStack() const;
    bool instructionStackEquals(Ins* reference) const;

    void dumpSettings();
    void dump();
};
