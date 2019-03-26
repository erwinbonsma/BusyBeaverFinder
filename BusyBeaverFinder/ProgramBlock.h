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

class ProgramBlock {
    bool _isDelta;
    int _instructionAmount;
    int _numSteps;

//    bool _valueOnEntryWasZero;

public:
    ProgramBlock();

    void init(bool isDelta, int amount, int numSteps, ProgramBlock* zeroBlock, ProgramBlock* nonZeroBlock);

    bool isDelta() { return _isDelta; }
    int getInstructionAmount() { return _instructionAmount; }
    int getNumSteps() { return _numSteps; }
};

#endif /* ProgramBlock_h */
