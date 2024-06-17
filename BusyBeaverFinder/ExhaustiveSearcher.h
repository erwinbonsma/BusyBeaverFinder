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
    int dataSize = 1024;

    // The maximum number of steps with hang detection enabled
    int maxHangDetectionSteps = 1024;

    // The maximum number of steps where the program space is searched during execution.
    // Beyond this limit, if a program encounters an unset instruction, it is a late escape.
    int maxSearchSteps = 1024;

    // The maximum steps that a program will run for.
    int maxSteps = 1024;

    bool testHangDetection = false;
    bool disableNoExitHangDetection = false;
};

class ExhaustiveSearcher {
    SearchSettings _settings;

    Program _program;

    // Determines when to abort the search
    SearchMode _searchMode;

    std::vector<Ins>::const_iterator _resumeIns;
    std::vector<Ins>::const_iterator _resumeEnd;
    bool _resuming;

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
