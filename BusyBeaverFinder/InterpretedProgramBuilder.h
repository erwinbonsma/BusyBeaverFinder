//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef InterpretedProgramBuilder_h
#define InterpretedProgramBuilder_h

#include <stdio.h>

#include "InterpretedProgram.h"
#include "Types.h"
#include "ProgramBlock.h"

const int maxProgramBlocks = maxWidth * maxHeight * 2;
const int maxProgramStackFrames = maxWidth * maxHeight;

class Program;

struct MutableProgramBlock {
    uint8_t flags;
    int8_t amount;
    int8_t numSteps;
};

struct ProgramStack {
    MutableProgramBlock activeBlock;
    int activeBlockIndex;
    int numBlocks;
    int numFinalizedBlocks;
};

class InterpretedProgramBuilder : public InterpretedProgram {
    ProgramBlock _blocks[maxProgramBlocks];
    int _blockIndexLookup[maxProgramBlocks];
    int _finalizedStack[maxProgramBlocks];

    ProgramStack _state[maxProgramStackFrames];
    ProgramStack* _stateP;

    ProgramBlock* getBlock(InstructionPointer insP, TurnDirection turn);
    InstructionPointer startInstructionForBlock(ProgramBlock* block);

    void checkState();

public:
    InterpretedProgramBuilder();

    // Implementation of InterpretedProgram
    int numProgramBlocks() const override { return _stateP->numBlocks; };
    const ProgramBlock* programBlockAt(int index) const override { return _blocks + index; };
    int indexOf(const ProgramBlock *block) const override { return (int)(block - _blocks); }

    void push();
    void pop();

    // Update the active program block
    int incSteps() { return _stateP->activeBlock.numSteps++; }
    void incAmount() { _stateP->activeBlock.amount++; }
    void decAmount() { _stateP->activeBlock.amount--; }
    void setInstruction(bool isDelta);

    bool isDeltaInstruction();
    bool isInstructionSet();
    int getAmount();
    int getNumSteps();

    ProgramBlock* getEntryBlock() { return _blocks; }
    ProgramBlock* finalizeBlock(InstructionPointer endP);
    ProgramBlock* enterBlock(ProgramBlock* block);
    ProgramBlock* enterBlock(InstructionPointer startP, TurnDirection turnDir);
    ProgramBlock* getBlock(int startIndex) { return _blocks + _blockIndexLookup[startIndex]; }

    TurnDirection startTurnDirectionForBlock(ProgramBlock* block);
    ProgramPointer getStartProgramPointer(ProgramBlock* block, Program& program);
};

#endif /* InterpretedProgramBuilder_h */
