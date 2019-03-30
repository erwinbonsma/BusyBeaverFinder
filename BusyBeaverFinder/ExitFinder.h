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

#include "CompiledProgram.h"

typedef ProgramBlock* ProgramBlockP;

class ExitFinder {
    // Tracks if the block with the given index has been visited already
    bool _visited[maxProgramBlocks];

    // The blocks still to visit
    ProgramBlockP _pendingStack[maxProgramBlocks];

    ProgramBlockP *_nextP;
    ProgramBlockP *_topP;

    // Returns "true" if escape from this block is possible
    bool visitBlock(ProgramBlock* block);

public:
    ExitFinder();

    // Returns "true" if escape from this block is possible
    bool canExitFrom(ProgramBlock* block);
};

#endif /* ExitFinder_h */
