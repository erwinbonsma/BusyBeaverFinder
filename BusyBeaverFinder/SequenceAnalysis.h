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
#include "DataDeltas.h"

class InterpretedProgram;
class ProgramBlock;
class RunSummary;

class SequenceAnalysis {

protected:
    std::vector<const ProgramBlock*> _programBlocks;

    int _dpDelta, _minDp, _maxDp;

    // The result of executing one sequence.
    //
    // Note, for loops the LoopAnalysis subclass will process these deltas so that they reflect
    // effective changes once the loop is fully spun up. I.e. changes in subsequent loop iterations
    // that cancel each other out have been squashed.
    DataDeltas _dataDeltas;

    // The result after executing an instruction in the sequence, relative to when it started.
    // It shows how much DP has shifted, and how much the value that DP now points at has changed.
    std::vector<DataDelta> _effectiveResult;

    virtual void analyzeSequence();

public:
    SequenceAnalysis();

    int sequenceSize() const { return (int)_programBlocks.size(); }
    const ProgramBlock* programBlockAt(int index) const { return _programBlocks[index]; }

    int dataPointerDelta() const { return _dpDelta; }

    const DataDeltas& dataDeltas() const { return _dataDeltas; }

    virtual int numDataDeltas() const { return _dataDeltas.numDeltas(); }
    virtual const DataDelta& dataDeltaAt(int index) const { return _dataDeltas.dataDelta(index); }

    const DataDelta& effectiveResultAt(int index) const { return _effectiveResult[index]; }

    // Returns "true" when there are any data deltas when the sequence is partially executed,
    // up until (inclusive) the specified instruction.
    bool anyDataDeltasUpUntil(int index) const;

    bool analyzeSequence(const ProgramBlock* entryBlock, int numBlocks);
    bool analyzeSequence(InterpretedProgram& program, const RunSummary& runSummary,
                         int startIndex, int length);
};

std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa);

#endif /* SequenceAnalysis_h */
