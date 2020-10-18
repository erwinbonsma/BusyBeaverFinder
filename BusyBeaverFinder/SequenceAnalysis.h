//
//  SequenceAnalysis.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef SequenceAnalysis_h
#define SequenceAnalysis_h

#include <iostream>

const int maxSequenceSize = 128;
const int maxDataDeltasPerSequence = 32;

class DataDelta {
    friend class SequenceAnalysis;
    friend class LoopAnalysis;

protected:
    int _dpOffset;
    int _delta;

    // Depending on usage, track a different index
    union {
        // For effectiveResult: Which instruction next changes this delta?
        int _maskedByIndex;
        // For dataDelta: Which instruction last changed this delta?
        int _lastUpdatedByIndex;
    };

    void init(int dpOffset) { _dpOffset = dpOffset; _delta = 0; }

    // Returns "true" if this results in a zero change (so that the delta can be removed).
    bool changeDelta(int delta) { _delta += delta; return _delta == 0; }

public:
    // Specifies the position of the data value relative to an assumed reference point
    int dpOffset() const { return _dpOffset; }

    // Specifies how much this value changes
    int delta() const { return _delta; }
};

class MaskedDataDelta : public DataDelta {
public:
    int maskedIndex() const { return _maskedByIndex; }
};

class InterpretedProgram;
class ProgramBlock;
class RunSummary;

class SequenceAnalysis {

protected:
    ProgramBlock* _programBlocks[maxSequenceSize];
    int _numBlocks;

    int _dpDelta, _minDp, _maxDp;

    // The result of executing one sequence.
    //
    // Note, for loops the LoopAnalysis subclass will process these deltas so that they reflect
    // effective changes once the loop is fully spun up. I.e. changes in subsequent loop iterations
    // that cancel each other out have been squashed.
    DataDelta _dataDelta[maxDataDeltasPerSequence];
    int _numDataDeltas;

    // The result after executing an instruction in the sequence, relative to when it started.
    // It shows how much DP has shifted, and how much the value that DP now points at has changed.
    MaskedDataDelta _effectiveResult[maxSequenceSize];

    int deltaAt(int dpOffset);

    // Updates and returns the effective delta at the specified data position.
    //
    // The index should specify the index of the instruction that performed this delta.
    int updateDelta(int dpOffset, int delta, int index);

    virtual void analyseSequence();

public:
    SequenceAnalysis();

    int sequenceSize() const { return _numBlocks; }
    const ProgramBlock* programBlockAt(int index) const { return _programBlocks[index]; }

    int dataPointerDelta() const { return _dpDelta; }

    int numDataDeltas() const { return _numDataDeltas; }
    const DataDelta& dataDeltaAt(int index) const { return _dataDelta[index]; }

    const MaskedDataDelta& effectiveResultAt(int index) const { return _effectiveResult[index]; }

    // Returns "true" when there are any data deltas when the sequence is partially executed,
    // up until (inclusive) the specified instruction.
    bool anyDataDeltasUpUntil(int index) const;

    bool analyseSequence(ProgramBlock* entryBlock, int numBlocks);
    bool analyseSequence(InterpretedProgram& program, RunSummary& runSummary,
                         int startIndex, int length);
};

std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa);

#endif /* SequenceAnalysis_h */
