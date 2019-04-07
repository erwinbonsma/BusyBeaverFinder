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
const int maxBranchPoints = 2;

class RunBlockSequence {
    friend class RunSummary;

    ProgramBlockIndex _programBlockIndex;
    int _sequenceIndex;

    int _leftChildIndex;
    int _rightChildIndex;

    void reset();

    // Returns "true" if this resulted in a loop
    bool addChild(ProgramBlockIndex programBlockIndex);

    void init(int parentIndex, int programBlockIndex);
};

class RunBlock {
    friend class RunSummary;

    int _startIndex;
    int _sequenceIndex;
    bool _isLoop;

    // Returns the index into program block stack of the run summary where this run block started.
    int getStartIndex() { return _startIndex; }

    RunBlock(int startIndex, int sequenceIndex, bool isLoop);

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

    // Stack of recently executed run blocks
    RunBlock* _runBlockHistory = nullptr;
    RunBlock* _runBlockHistoryMaxP = nullptr;

    // Pointer to current top of the stack
    RunBlock* _runBlockHistoryP = nullptr;

public:
    // Set the capacity (in program blocks)
    void setCapacity(int capacity);

    // Returns true if this resulted in the creation of one or more RunBlocks
    bool recordProgramBlock(ProgramBlock block);

    // The number of run blocks
    int getLength();

    RunBlock runBlockAt(int index);

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
