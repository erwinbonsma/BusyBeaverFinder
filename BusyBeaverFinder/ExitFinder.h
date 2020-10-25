//
//  ExitFinder.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ExitFinder_h
#define ExitFinder_h

#include <stdio.h>

#include "Program.h"
#include "InterpretedProgramBuilder.h"

typedef ProgramBlock* ProgramBlockP;

class ExitFinder {
    Program& _program;
    InterpretedProgramBuilder& _interpretedProgramBuilder;
    int _maxSteps;

    // Tracks if the block with the given index has been visited already
    bool _visited[maxProgramBlocks];

    // The blocks still to visit
    ProgramBlockP _pendingStack[maxProgramBlocks];
    ProgramBlockP *_nextP;
    ProgramBlockP *_topP;

    // The possible exits
    ProgramBlockP _exits[maxProgramBlocks];
    int _numExits;

    // Recursively checks if a given value (zero or non-zero) can be obtained via at least one
    // possible entry path.
    bool isPossibleExitValue(ProgramBlock* block, bool zeroValue, int delta, int depth);

    // Performs a simple analysis to check if the given block can be reached.
    bool isReachable(ProgramBlock* block);

    // Tries to finalize the current block.
    // Returns "true" if the block could be finalized (based on the currently set instructions)
    bool finalizeBlock(ProgramBlock* block);

    // Returns "true" if escape from this block is possible
    bool visitBlock(ProgramBlock* block);

public:
    ExitFinder(Program& program, InterpretedProgramBuilder& interpretedProgramBuilder);

    // Checks if it is possible to exit from the loop that starts with the given block. It will
    // finalize blocks that are not yet finalized but which can be finalized given the instructions
    // that are currently set.
    //
    // Returns "true" if escape from this block is possible (i.e. blocks can be reached that are
    // not yet finalized)
    bool canExitFrom(ProgramBlock* block);
};

#endif /* ExitFinder_h */
