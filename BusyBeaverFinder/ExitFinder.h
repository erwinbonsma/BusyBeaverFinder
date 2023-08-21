//
//  ExitFinder.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/03/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <vector>

#include "Program.h"
#include "InterpretedProgramBuilder.h"

class ExitFinder {
    Program& _program;
    InterpretedProgramBuilder& _programBuilder;
    int _maxSteps;

    // Tracks if the block with the given index has been visited already
    bool _visited[maxProgramBlocks];

    // The blocks still to visit
    std::vector<const ProgramBlock*> _visitStack;

    // The possible exits
    std::vector<const ProgramBlock*> _exits;

    // Recursively checks if a given value (zero or non-zero) can be obtained via at least one
    // possible entry path.
    bool isPossibleExitValue(const ProgramBlock* block, bool zeroValue, int delta, int depth);

    // Performs a simple analysis to check if the given block can be reached.
    bool isReachable(const ProgramBlock* block);

    void visitBlock(const ProgramBlock* block);

public:
    ExitFinder(Program& program, InterpretedProgramBuilder& programBuilder);

    // Checks if it is possible to exit from the loop that starts with the given block. It will
    // finalize blocks that are not yet finalized but which can be finalized given the instructions
    // that are currently set.
    //
    // Returns "true" if escape from this block is possible (i.e. blocks can be reached that are
    // not yet finalized)
    bool canExitFrom(const ProgramBlock* block);
};
