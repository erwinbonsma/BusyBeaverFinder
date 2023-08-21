//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include "Consts.h"
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

/* Maintains an InterpretedProgram for the program state as it is during the search. It updates it
 * as the search expands and backtracks.
 */
class InterpretedProgramBuilder : public InterpretedProgram {
    ProgramBlock _blocks[maxProgramBlocks];
    int _blockIndexLookup[maxProgramBlocks];
    int _finalizedStack[maxProgramBlocks];

    ProgramStack _state[maxProgramStackFrames];
    ProgramStack* _stateP;

    ProgramBlock* getBlock(InstructionPointer insP, TurnDirection turn);
    InstructionPointer startInstructionForBlock(const ProgramBlock* block);

    void checkState();

    bool isDeltaInstruction();

public:
    static InterpretedProgramBuilder fromProgram(Program& program);

    InterpretedProgramBuilder();

    //---------------------------------------------------------------------------------------------
    // Implementation of InterpretedProgram

    int numProgramBlocks() const override { return _stateP->numBlocks; };
    const ProgramBlock* programBlockAt(int index) const override { return _blocks + index; };
    int indexOf(const ProgramBlock *block) const override { return (int)(block - _blocks); }

    const ProgramBlock* getEntryBlock() const override { return _blocks; }

    //---------------------------------------------------------------------------------------------
    // Methods to update the program as the search progresses

    void push();
    void pop();

    // Update current block
    void addDataInstruction(Dir dir);
    int incSteps() { return _stateP->activeBlock.numSteps++; }

    bool isInstructionSet();
    int getAmount();
    int getNumSteps();

    const ProgramBlock* buildActiveBlock(Program& program);

    const ProgramBlock* finalizeExitBlock();
    const ProgramBlock* finalizeHangBlock();
    const ProgramBlock* finalizeBlock(InstructionPointer endP);

    const ProgramBlock* enterBlock(const ProgramBlock* block);
    const ProgramBlock* enterBlock(InstructionPointer startP, TurnDirection turnDir);
    ProgramBlock* getBlock(int startIndex) { return _blocks + _blockIndexLookup[startIndex]; }

    TurnDirection turnDirectionForBlock(const ProgramBlock* block);
    ProgramPointer getStartProgramPointer(const ProgramBlock* block, Program& program);
};
