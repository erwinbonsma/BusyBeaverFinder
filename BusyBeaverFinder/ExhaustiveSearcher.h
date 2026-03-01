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

#include "Searcher.h"
#include "Program.h"
#include "Resumer.h"

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

struct SearchSettings : public BaseSearchSettings {
    // The maximum number of steps with hang detection enabled
    int maxHangDetectionSteps = 1024;

    // The maximum number of steps where the program space is searched during execution.
    // Beyond this limit, if a program encounters an unset instruction, it is a late escape.
    int maxSearchSteps = 1024;


    bool testHangDetection = false;
    bool disableNoExitHangDetection = false;
};

class ExhaustiveSearcher : public Searcher {
    SearchSettings _settings;

    // Determines when to abort the search
    SearchMode _searchMode;

    std::unique_ptr<Resumer> _resumer;

    TurnDirection _td;
    ProgramPointer _pp;

    // Stack of instructions built up by the exhaustive search
    std::vector<Ins> _instructionStack;

    // An interpreted representation of the program
    std::shared_ptr<InterpretedProgramBuilder> _programBuilder;

    FastExecutor _fastExecutor;
    HangExecutor _hangExecutor;
    // Points to the active executor
    ProgramExecutor* _programExecutor;

    ExitFinder _exitFinder;

    void verifyHang();

    void run();
    void branch();
    void extendBlock();
    void buildBlock(const ProgramBlock* block);

    void switchToHangExecutor();

public:
    ExhaustiveSearcher(SearchSettings settings);

    bool getHangDetectionTestMode() const { return _settings.testHangDetection; }

    std::shared_ptr<const InterpretedProgram> getInterpretedProgram() const {
        return _programBuilder;
    }

    int getNumSteps() const override { return _programExecutor->numSteps(); }
    const ProgramExecutor* getProgramExecutor() const { return _programExecutor; }

    //----------------------------------------------------------------------------------------------

    bool atTargetProgram();

    void search();

    // Starts searching once the resume stack is empty _and_ the number of executed steps exceeds
    // fromSteps. It is an error when an unset instruction is encountered when the resume stack is
    // empty but the target step count is not yet reached.
    void search(std::unique_ptr<Resumer> resumer, int fromSteps = 0);

    void searchSubTree(const std::vector<Ins> &resumeFrom, int fromSteps = 0);

    // Executes the program specified by the given spec until the first UNSET instruction is
    // encountered. Searches the sub-tree from that point onwards. This is mainly used to follow up
    // on late escapes.
    void searchSubTree(const std::string& programSpec, int fromSteps = 0);

    void findOne();
    void findOne(const std::vector<Ins> &resumeFrom);

    void dumpInstructionStack(const std::string& sep = {}) const;
    bool instructionStackEquals(Ins* reference) const;

    void dumpSearchProgress(std::ostream &os) const override;

    void dumpSettings(std::ostream &os) const override;
    void dump();
};
