//
//  LoopClassification.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef LoopClassification_h
#define LoopClassification_h

#include <stdio.h>

class InterpretedProgram;
class RunSummary;
class ProgramBlock;

const int maxDataDeltasPerLoop = 4;

class DataDelta {
    friend class LoopClassification;

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

//class LoopExit {
//public:
//    Condition exitCondition();
//};

class LoopClassification {
    int _dpDelta;
    DataDelta _dataDelta[maxDataDeltasPerLoop];
    int _numDataDeltas;

    void updateDelta(int dpOffset, int delta);

    // Determine the effective delta over multiple iterations, taking into account the shifting DP
    void squashDeltas();

public:
    LoopClassification();

    int dataPointerDelta() { return _dpDelta; }

    int numDataDeltas() { return _numDataDeltas; }
    DataDelta* dataDeltaAt(int index) { return _dataDelta + index; }

    void classifyLoop(ProgramBlock* entryBlock, int numBlocks);

    //void classifyLoop(InterpretedProgram& program, RunSummary& runSummary, int runBlockIndex);

    void dump();
};

#endif /* LoopClassification_h */
