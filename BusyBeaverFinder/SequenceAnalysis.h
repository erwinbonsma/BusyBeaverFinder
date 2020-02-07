//
//  SequenceAnalysis.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef SequenceAnalysis_h
#define SequenceAnalysis_h

#include <stdio.h>

const int maxSequenceSize = 128;
const int maxDataDeltasPerSequence = 32;

class DataDelta {
    friend class SequenceAnalysis;
    friend class LoopAnalysis;

    int _dpOffset;
    int _delta;

    void init(int dpOffset) { _dpOffset = dpOffset; _delta = 0; }

    // Returns "true" if this results in a zero changes (so that the delta can be removed).
    bool changeDelta(int delta) { _delta += delta; return _delta == 0; }

public:
    // Specifies the position of the data value relative to an assumed reference point
    int dpOffset() { return _dpOffset; }

    // Specifies how much this value changes
    int delta() { return _delta; }
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
    DataDelta _effectiveResult[maxSequenceSize];

    int deltaAt(int dpOffset);

    // Updates and returns the effective delta at the specified data position.
    int updateDelta(int dpOffset, int delta);

    virtual void analyseSequence();

public:
    SequenceAnalysis();

    int sequenceSize() { return _numBlocks; }

    int dataPointerDelta() { return _dpDelta; }

    int numDataDeltas() { return _numDataDeltas; }
    DataDelta* dataDeltaAt(int index) { return _dataDelta + index; }

    bool analyseSequence(ProgramBlock* entryBlock, int numBlocks);
    bool analyseSequence(InterpretedProgram& program, RunSummary& runSummary,
                         int startIndex, int length);

    void dump();
};

#endif /* SequenceAnalysis_h */
