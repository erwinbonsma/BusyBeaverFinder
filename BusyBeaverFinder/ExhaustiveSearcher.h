//
//  ExhaustiveSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <stdint.h>
#include <iterator>
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
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

const SearchSettings defaultSearchSettings = {
    .dataSize = 1024,
    .maxSteps = 1024,
    .maxHangDetectionSteps = 1024,
    .testHangDetection = false,
    .disableNoExitHangDetection = false
};

class ExhaustiveSearcher {
    SearchSettings _settings;

    Program _program;

    // Determines when to abort the search
    SearchMode _searchMode;

    std::vector<Ins>::const_iterator _resumeIns;
    std::vector<Ins>::const_iterator _resumeEnd;

    TurnDirection _td;
    ProgramPointer _pp;

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

    void verifyHang();

    void run();
    void branch();
    void extendBlock();
    void buildBlock(const ProgramBlock* block);
public:
    ExhaustiveSearcher(int width, int height, SearchSettings settings);

    SearchSettings getSettings() const { return _settings; }

    bool getHangDetectionTestMode() const { return _settings.testHangDetection; }

    ProgressTracker* getProgressTracker() const { return _tracker; }
    void setProgressTracker(ProgressTracker* tracker);

    const Program& getProgram() const { return _program; }
    const InterpretedProgram& getInterpretedProgram() const { return _programBuilder; }

    const ProgramExecutor* getProgramExecutor() const { return _programExecutor; }

    //----------------------------------------------------------------------------------------------

    bool atTargetProgram();

    void search();
    void search(const std::vector<Ins> &resumeFrom);

    void searchSubTree(const std::vector<Ins> &resumeFrom);

    void findOne();
    void findOne(const std::vector<Ins> &resumeFrom);

    void dumpInstructionStack() const;
    bool instructionStackEquals(Ins* reference) const;

    void dumpSettings();
    void dump();
};
