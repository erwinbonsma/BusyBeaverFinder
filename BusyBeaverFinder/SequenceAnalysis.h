//
//  SequenceAnalysis.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "DataDeltas.h"
#include "ProgramBlock.h"

class InterpretedProgram;
class RunSummary;

class PreCondition {
    int _value;
    bool _shouldEqual;

public:
    PreCondition(int value, bool shouldEqual) : _value(value), _shouldEqual(shouldEqual) {}

    int value() const { return _value; }
    bool shouldEqual() const { return _shouldEqual; }

    bool holdsForValue(int value) const;

    bool operator==(const PreCondition& rhs) const {
        return _value == rhs._value && _shouldEqual == rhs._shouldEqual;
    }
};

typedef const ProgramBlock *const * RawProgramBlocks;

class SequenceAnalysis {
    friend std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa);

    const ProgramBlock* _prevProgramBlock;
protected:
    int _dpDelta, _minDp, _maxDp;

    // The sequence that is analyzed. It is only valid while the sequence is analyzed, as it is a
    // pointer into vector-managed memory, which can be re-allocated when more program blocks are
    // added to the run history.
    RawProgramBlocks _programBlocks;
    int _numProgramBlocks;

    // The result of executing one sequence.
    //
    // Note, for loops the LoopAnalysis subclass will process these deltas so that they reflect
    // effective changes once the loop is fully spun up. I.e. changes in subsequent loop iterations
    // that cancel each other out have been squashed.
    DataDeltas _dataDeltas;

    // The result after executing an instruction in the sequence, relative to when it started.
    // It shows how much DP has shifted, and how much the value that DP now points at has changed.
    std::vector<DataDelta> _effectiveResult;

    // The conditions that must hold with respect to the data for this sequence to be fully
    // executed. Keys are DP offsets
    std::multimap<int, PreCondition> _preConditions;

    // Returns true if analysis can proceed
    virtual bool startAnalysis();

    void analyzeBlock(const ProgramBlock* block);

    // Returns true is analysis completed and the sequence matched the expected pattern
    virtual bool finishAnalysis();

    void addPreCondition(int dpOffset, PreCondition preCondition);

    virtual const char* typeString() const { return "SEQ"; }

public:
    SequenceAnalysis();

    int sequenceSize() const { return _numProgramBlocks; }

    int dataPointerDelta() const { return _dpDelta; }
    int minDp() const { return _minDp; }
    int maxDp() const { return _maxDp; }

    virtual const DataDeltas& dataDeltas() const { return _dataDeltas; }

    virtual int numDataDeltas() const { return _dataDeltas.numDeltas(); }
    virtual const DataDelta& dataDeltaAt(int index) const { return _dataDeltas.dataDelta(index); }

    const DataDelta& effectiveResultAt(int index) const { return _effectiveResult[index]; }

    const std::multimap<int, PreCondition> preConditions() const { return _preConditions; }
    bool hasPreCondition(int dpOffset, PreCondition preCondition) const;

    bool analyzeSequence(RawProgramBlocks programBlocks, int len);

    void dump() const { std::cout << *this << std::endl; }
};

std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa);
