//
//  InterpretedProgram.h
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <array>

#include "InterpretedProgram.h"
#include "Types.h"
#include "Program.h"
#include "ProgramBlock.h"

// A power of two for efficient indexing
constexpr int maxProgramSize = 8;
constexpr int maxProgramBlocks = maxProgramSize * maxProgramSize * 2;

struct MutableProgramBlockProps {
    uint8_t flags;
    int8_t amount;
    int8_t numSteps;
};

struct ProgramStack {
    ProgramStack(int numActivatedAtStart, int numFinalizedAtStart)
    : numActivatedAtStart(numActivatedAtStart), numFinalizedAtStart(numFinalizedAtStart) {}

    ProgramStack(int numActivatedAtStart, int numFinalizedAtStart,
                 ProgramBlock* activeBlock, MutableProgramBlockProps activeProps) :
        numActivatedAtStart(numActivatedAtStart),
        numFinalizedAtStart(numFinalizedAtStart),
        activeBlock(activeBlock),
        activeProps(activeProps) {}

    int numActivatedAtStart = 0;
    int numFinalizedAtStart = 0;

    MutableProgramBlockProps activeProps;
    ProgramBlock *activeBlock = nullptr;
};

/* Maintains an InterpretedProgram for the program state as it is during the search. It updates it
 * as the search expands and backtracks.
 */
class InterpretedProgramBuilder : public InterpretedProgram {
    ProgramSize _size;

    // All program blocks. They are indexed by startIndex. Initially none are finalized. They are
    // finalized as needed.
    std::array<ProgramBlock, maxProgramBlocks> _blocks;

    // Look-up from start index to block index. Returns -1 if block is not yet activated.
    std::array<int, maxProgramBlocks> _blockIndexLookup;

    // Stack with program blocks that have been activated (by visiting them via getBlock)
    std::vector<ProgramBlock*> _activatedStack;

    // Stack with finalized rogram blocks. This is used to undo finalization on back-tracking
    std::vector<ProgramBlock*> _finalizedStack;

    // Maintains stack of states to enable back-tracking.
    std::vector<ProgramStack> _state;

    ProgramBlock* getBlock(InstructionPointer insP, TurnDirection turn);
    InstructionPointer startInstructionForBlock(const ProgramBlock* block);

    void reset();

    bool isDeltaInstruction();

public:
    InterpretedProgramBuilder();

    // Helper method to build the interpreted program from the supplied program
    void buildFromProgram(Program& program);

    //---------------------------------------------------------------------------------------------
    // Implementation of InterpretedProgram

    int indexOf(const ProgramBlock *block) const override {
        return _blockIndexLookup[block->getStartIndex()];
    }
    int numProgramBlocks() const override { return static_cast<int>(_activatedStack.size()); };
    const ProgramBlock* programBlockAt(int index) const override { return _activatedStack[index]; };

    const ProgramBlock* getEntryBlock() const override { return &_blocks[0]; }

    //---------------------------------------------------------------------------------------------
    // Methods to update the program as the search progresses

    void push();
    void pop();

    // Update current block
    void addDataInstruction(Dir dir);
    int incSteps() { return _state.back().activeProps.numSteps++; }

    bool isInstructionSet();
    int getAmount();
    int getNumSteps();

    const ProgramBlock* buildActiveBlock(Program& program);

    const ProgramBlock* finalizeExitBlock();
    const ProgramBlock* finalizeHangBlock();
    const ProgramBlock* finalizeBlock(InstructionPointer endP);

    const ProgramBlock* enterBlock(const ProgramBlock* block);
    const ProgramBlock* enterBlock(InstructionPointer startP, TurnDirection turnDir);
    ProgramBlock* getBlock(int startIndex) { return &_blocks[startIndex]; }

    TurnDirection turnDirectionForBlock(const ProgramBlock* block);
    ProgramPointer getStartProgramPointer(const ProgramBlock* block, Program& program);
};
