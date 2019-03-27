//
//  ProgramBlock.h
//  BusyBeaverFinder
//
//  Created by Erwin on 25/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ProgramBlock_h
#define ProgramBlock_h

#include <stdio.h>
#include <assert.h>

#include "Types.h"

class ProgramBlock {
    int _startIndex;
    ProgramBlock* _zeroBlock;
    ProgramBlock* _nonZeroBlock;

    bool _isFinalized;
    bool _isDelta;
    int _instructionAmount;
    int _numSteps;

public:
    ProgramBlock();

    void init(int startIndex);
    void finalize(bool isDelta, int amount, int numSteps,
                  ProgramBlock* zeroBlock, ProgramBlock* nonZeroBlock);

    // Index that uniquely specifies the starting position (including turn direction)
    int getStartIndex() { return _startIndex; }

    bool isFinalized() { return _isFinalized; }

    bool isDelta() { return _isDelta; }
    int getInstructionAmount() { return _instructionAmount; }
    int getNumSteps() { return _numSteps; }

    ProgramBlock* zeroBlock() { return _zeroBlock; }
    ProgramBlock* nonZeroBlock() { return _nonZeroBlock; }
};

#endif /* ProgramBlock_h */
