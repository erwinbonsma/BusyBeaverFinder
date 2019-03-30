//
//  ExitFinder.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExitFinder.h"

#include <iostream>

#include "Utils.h"

ExitFinder::ExitFinder(Program& program, CompiledProgram& compiledProgram) :
    _program(program),
    _compiledProgram(compiledProgram)
{
    for (int i = 0; i < maxProgramBlocks; i++) {
        _visited[i] = false;
    }

    // Bound on the maximum number of steps to execute within a block without processing a DATA
    // instruction before concluding that the block loops into itself. This is not an exact bound
    // but should be high enough. (If not, it will be discovered as this program itself will end up
    // in an endless loop).
    _maxSteps = (program.getWidth() - 1) * (program.getHeight() - 1);
}

ProgramPointer ExitFinder::getStartProgramPointer(ProgramBlock* block) {
    ProgramPointer pp;

    pp.p = _compiledProgram.startInstructionForBlock(block);
    pp.dir = (Dir)0;

    // Find a TURN
    while (_program.getInstruction(nextInstructionPointer(pp)) != Ins::TURN) {
        pp.dir = (Dir)((int)pp.dir + 1);
    }

    return pp;
}

bool ExitFinder::finalizeBlock(ProgramBlock* block) {
    _compiledProgram.enterBlock(block);

    ProgramPointer pp = getStartProgramPointer(block);
    int steps = 0;

    // Rotation delta
    int delta = (_compiledProgram.startTurnDirectionForBlock(block) == TurnDirection::CLOCKWISE)
        ? 1 : 3;

    while (1) {
processInstruction:
        InstructionPointer insP = nextInstructionPointer(pp);

        switch (_program.getInstruction(insP)) {
            case Ins::DATA:
                switch (pp.dir) {
                    case Dir::UP:
                        _compiledProgram.setInstruction(true);
                        _compiledProgram.incAmount();
                        break;
                    case Dir::DOWN:
                        _compiledProgram.setInstruction(true);
                        _compiledProgram.decAmount();
                        break;
                    case Dir::RIGHT:
                        _compiledProgram.setInstruction(false);
                        _compiledProgram.incAmount();
                        break;
                    case Dir::LEFT:
                        _compiledProgram.setInstruction(false);
                        _compiledProgram.decAmount();
                        break;
                }
                break;
            case Ins::NOOP:
                break;
            case Ins::DONE:
            case Ins::UNSET:
                // Escaped from loop. So cannot conclude that program hangs
                return false;
            case Ins::TURN:
                if (_compiledProgram.isInstructionSet()) {
                    _compiledProgram.finalizeBlock(pp.p);
                    return true;
                } else {
                    pp.dir = (Dir)(((int)pp.dir + delta) % 4);
                    goto processInstruction;
                }
                break;
        }

        if (steps++ > _maxSteps) {
            // Apparently the block itself is in an endless loop. For purposes of No Exit hang
            // detection, just finalize it. We cannot conclude here that the program hangs, as there
            // is no guarantee that this block will be entered.
            //
            // Note, setting an instruction so that the next TURN will finalize the block. This
            // ensures that the block is finalized at a TURN, as it should.
            _compiledProgram.setInstruction(true);
        }
        pp.p = insP;
    }
}

bool ExitFinder::visitBlock(ProgramBlock* block) {
    if (block == nullptr) {
        // This is a block that cannot be entered
        return false;
    }

    if (_visited[block->getStartIndex()]) {
        // Already visited this block
        return false;
    }

    if (!block->isFinalized()) {
        if (!finalizeBlock(block)) {
            // The block cannot yet be finalized.
            return true;
        }
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
