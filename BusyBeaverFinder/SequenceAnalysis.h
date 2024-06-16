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

    // Should normally not happen. Can happen in multi-sequence analysis for instructions that
    // spill outside the DP range of intereset.
    bool _invalid = false;

public:
    PreCondition(int value, bool shouldEqual) : _value(value), _shouldEqual(shouldEqual) {}

    int value() const { return _value; }
    bool shouldEqual() const { return _shouldEqual; }

    void invalidate() { _invalid = true; }
    bool isValid() const { return !_invalid; }

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
    int _dpDelta {};
    int _minDp {};
    int _maxDp {};

    // The sequence that is analyzed. It is only valid while the sequence is analyzed, as it is a
    // pointer into vector-managed memory, which can be re-allocated when more program blocks are
    // added to the run history.
    //
    // TODO: Remove once SweepLoopAnalysis is removed
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
    std::vector<DataDelta> _effectiveResult {32};

    // The conditions that must hold with respect to the data for this sequence to be fully
    // executed. Keys are DP offsets
    std::multimap<int, PreCondition> _preConditions;

    virtual void startAnalysis();

    void analyzeBlock(const ProgramBlock* block);
    virtual void analyzeBlocks(RawProgramBlocks programBlocks, int len);

    // Returns true is analysis completed and the sequence matched the expected pattern
    virtual bool finishAnalysis();

    void addPreCondition(int dpOffset, PreCondition preCondition);

    virtual const char* typeString() const { return "SEQ"; }

public:
    SequenceAnalysis() = default;
    SequenceAnalysis(const SequenceAnalysis&) = delete;
    SequenceAnalysis& operator=(const SequenceAnalysis&) = delete;

    virtual bool isLoop() const { return false; }
    int sequenceSize() const { return _numProgramBlocks; }

    int dataPointerDelta() const { return _dpDelta; }

    int minDp() const { return _minDp; }
    int maxDp() const { return _maxDp; }

    const DataDeltas& dataDeltas() const { return _dataDeltas; }

    const DataDelta& effectiveResultAt(int index) const { return _effectiveResult[index]; }

    const std::multimap<int, PreCondition> preConditions() const { return _preConditions; }
    bool hasPreCondition(int dpOffset, PreCondition preCondition) const;

    // Analyse the given continous sequence of program blocks
    bool analyzeSequence(RawProgramBlocks programBlocks, int len);

    // Analyse the combined effects of multiple sub-sequences
    void analyzeMultiSequenceStart();
    // dpDelta gives the (relative) start position of DP at start of program block sequence
    void analyzeMultiSequence(RawProgramBlocks programBlocks, int len, int dpDelta);
    bool analyzeMultiSequenceEnd(int dpDelta);

    void dump() const { std::cout << *this << std::endl; }
};

std::ostream &operator<<(std::ostream &os, const SequenceAnalysis &sa);
