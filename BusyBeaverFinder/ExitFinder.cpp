//
//  ExitFinder.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExitFinder.h"


ExitFinder::ExitFinder() {
    for (int i = 0; i < maxProgramBlocks; i++) {
        _visited[i] = false;
    }
}

bool ExitFinder::visitBlock(ProgramBlock* block) {
    if (_visited[block->getStartIndex()]) {
        // Already visited this block
        return false;
    }

    if (!block->isFinalized()) {
        // Not yet finalized. So can escape from current program.
        return true;
    }

    // Add to stack
    *(_topP++) = block;
    _visited[block->getStartIndex()] = true;
    return false;
}

bool ExitFinder::canExitFrom(ProgramBlock* block) {
    _nextP = _pendingStack;
    _topP = _pendingStack;

    bool escapedFromLoop = visitBlock(block);

    while (!escapedFromLoop && _nextP < _topP) {
        ProgramBlock* block = *_nextP++;

        escapedFromLoop = (
            visitBlock(block->zeroBlock()) ||
            visitBlock(block->nonZeroBlock())
        );
    }

    // Reset tracking state
    while (_topP > _pendingStack) {
        _visited[ (*--_topP)->getStartIndex() ] = false;
    }

    return escapedFromLoop;

}
