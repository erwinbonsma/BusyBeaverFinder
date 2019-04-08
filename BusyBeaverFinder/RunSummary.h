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

typedef char ProgramBlockIndex;

// Should be set such that it never is the limiting factor for hang detection (instead, the logic
// of the available/implemented hang detectors should be)
const int maxRunBlockHistoryLength = 1024;

// Should both be set such that is is never the limiting factor for hang detection
const int maxSequenceStarts = 16;
const int numSequenceBlocks = 512;


class RunBlockSequence {
    friend class RunSummary;

    ProgramBlockIndex _programBlockIndex;

    int _children[2];

    // Creates child as needed
    void getChild(ProgramBlockIndex programBlockIndex);

    void init(ProgramBlockIndex programBlockIndex);

    ProgramBlockIndex getProgramBlockIndex() { return _programBlockIndex; }
};

class RunBlock {
    friend class RunSummary;

    int _startIndex;
    int _sequenceIndex;
    bool _isLoop;

    // Returns the index into program block stack of the run summary where this run block started.
    int getStartIndex() { return _startIndex; }

    void init(int startIndex, int sequenceIndex, bool isLoop);

public:
    bool isLoop() { return _isLoop; }

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

    // Pointer to current top of the stack
    RunBlock* _runBlockHistoryP = nullptr;

    RunBlockSequence _sequenceBlock[numSequenceBlocks];
    int _numSequenceStarts;
    int _nextSequenceIndex;

    void freeDynamicArrays();

    // Returns the length of the loop if one is detected. Returns zero otherwise.
    int detectLoop();

    RunBlockSequence* findSequenceStart(ProgramBlockIndex targetIndex);
    int getSequenceIndex(ProgramBlockIndex* startP, ProgramBlockIndex* endP);

    // Creates a run block for the programs blocks from startP (inclusive) to endP (exclusive)
    void createRunBlock(ProgramBlockIndex* startP, ProgramBlockIndex* endP, bool isLoop);

public:
    ~RunSummary();

    void reset();

    // Set the capacity (in program blocks)
    void setCapacity(int capacity);

    // Returns true if this resulted in the creation of one or more RunBlocks
    bool recordProgramBlock(ProgramBlockIndex blockIndex);

    bool isInsideLoop() { return _loopP != nullptr; }
    int getLoopPeriod() { return (int)(_programBlockHistoryP - _loopP) - 1; }

    int getNumProgramBlocks() { return (int)(_programBlockHistoryP - _programBlockHistory); }
    int getNumRunBlocks() { return (int)(_runBlockHistoryP - _runBlockHistory); }

    RunBlock* runBlockAt(int index) { return _runBlockHistory + index; }

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

    void dump();
};

#endif /* RunSummary_h */
