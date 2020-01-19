//
//  RunSummary.h
//  BusyBeaverFinder
//
//  Created by Erwin on 07/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef RunSummary_h
#define RunSummary_h

#include <stdio.h>

class ProgramBlock;

typedef int ProgramBlockIndex;

// The next to constants should be set such that it never is the limiting factor for hang detection
// (instead, the logic of the available/implemented hang detectors should be)
#ifdef DEBUG
const int maxRunBlockHistoryLength = 32768;
const int maxNumSequenceBlocks = 8192;
#else
const int maxRunBlockHistoryLength = 1000000; // TODO: Make dynamic
const int maxNumSequenceBlocks = 65536;
#endif


class RunBlockSequenceNode {
    friend class RunSummary;

    ProgramBlockIndex _programBlockIndex;

    int _childIndex;
    int _siblingIndex;

    void init(ProgramBlockIndex programBlockIndex);

    ProgramBlockIndex getProgramBlockIndex() { return _programBlockIndex; }
};

class RunBlock {
    friend class RunSummary;

    int _startIndex;
    int _sequenceIndex;
    int _loopPeriod;

    void init(int startIndex, int sequenceIndex, int loopPeriod);

public:
    bool isLoop() { return _loopPeriod != 0; }
    int getLoopPeriod() { return _loopPeriod; }

    // Returns the index into program block stack of the run summary where this run block started.
    int getStartIndex() { return _startIndex; }

    // Returns a unique identifier for this (type of) run block. Different run blocks have the same
    // index when:
    // - their execution sequences match exactly, or
    // - they represent the same loop. In this case, the number of loop iterations may differ
    int getSequenceIndex() { return _sequenceIndex; }
};

class RunSummary {
    friend class RunBlock;

    // Stack of recently executed program blocks
    ProgramBlockIndex* _programBlockHistory = nullptr;
    ProgramBlockIndex* _programBlockHistoryMaxP = nullptr;

    // Pointer to current top of the stack
    ProgramBlockIndex* _programBlockHistoryP = nullptr;

    // Pointer to first program block that is not yet part of a run block
    ProgramBlockIndex* _programBlockPendingP = nullptr;

    // Pointer to the previous iteration of the loop when in a loop. Otherwise equals nullptr
    ProgramBlockIndex* _loopP = nullptr;

    // Stack of recently executed run blocks
    RunBlock _runBlockHistory[maxRunBlockHistoryLength];

    RunBlock* _runBlockHistoryMaxP = _runBlockHistory + maxRunBlockHistoryLength;

    // Pointer to threshold where buffer is considered full. As a single invocation of
    // recordProgramBlock can add multiple run blocks, there may still be a few empty spots after
    // the threshold.
    RunBlock* _runBlockHistoryThresholdP = _runBlockHistory + maxRunBlockHistoryLength - 1;

    // Pointer to current top of the stack
    RunBlock* _runBlockHistoryP = nullptr;

    RunBlockSequenceNode _sequenceBlock[maxNumSequenceBlocks];
    int _numSequenceBlocks;

    // Helper array required by findRepeatedSequence utility function
    // Note: It is not owned by this class (and should therefore not be freed by it)
    int* _helperBuf;

    void freeDynamicArrays();

    RunBlockSequenceNode* getChildNode(RunBlockSequenceNode* parent, ProgramBlockIndex targetIndex);
    int getSequenceIndex(ProgramBlockIndex* startP, ProgramBlockIndex* endP);

    // Creates a run block for the programs blocks from startP (inclusive) to endP (exclusive)
    void createRunBlock(ProgramBlockIndex* startP, ProgramBlockIndex* endP, int loopPeriod);

    void dumpRunBlockSequenceNode(RunBlockSequenceNode* node, int level);

public:
    ~RunSummary();

    void reset();

    // Set the capacity (in program blocks)
    void setCapacity(int capacity, int* helperBuf);

    // Returns true if this resulted in the creation of one or more RunBlocks
    bool recordProgramBlock(ProgramBlockIndex blockIndex);

    bool isInsideLoop() { return _loopP != nullptr; }
    int getLoopPeriod() { return (int)(_programBlockHistoryP - _loopP); }

    // Returns true when the loop just completed an iteration
    bool isAtEndOfLoop();

    // Returns true when the loop will start a next iteration
    bool isAtStartOfLoop(ProgramBlockIndex nextBlockIndex) {
        return isAtEndOfLoop() && *_loopP == nextBlockIndex;
    }

    bool loopContinues(ProgramBlockIndex nextBlockIndex) {
        return *_loopP == nextBlockIndex;
    }

    bool hasSpaceRemaining() { return _runBlockHistoryP < _runBlockHistoryThresholdP; }

    int getNumProgramBlocks() { return (int)(_programBlockHistoryP - _programBlockHistory); }
    int getNumRunBlocks() { return (int)(_runBlockHistoryP - _runBlockHistory); }

    ProgramBlockIndex programBlockIndexAt(int index) { return _programBlockHistory[index]; }
    RunBlock* runBlockAt(int index) { return _runBlockHistory + index; }
    RunBlock* getLastRunBlock() { return (_runBlockHistoryP - 1); }

    // Returns the length in program blocks of the given run block.
    //
    // Note, for loops it counts the repeated executions. Furthermore, the total does not have to
    // be a multiple of the length of the sequence, as the loop may break from anywhere in the
    // sequence.
    //
    // E.g.
    // Seq = A B C (where A, B and C are Program Blocks)
    // => A B C A B C A B X => length = 8, where X is the program block that breaks the loop
    int getRunBlockLength(int index);

    void dumpSequenceTree();
    void dumpCondensed();
    void dump();
};

#endif /* RunSummary_h */
