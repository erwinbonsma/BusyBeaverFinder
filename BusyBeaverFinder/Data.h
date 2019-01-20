//
//  Data.hpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#ifndef Data_h
#define Data_h

#include <stdio.h>

#include "Consts.h"
#include "Enums.h"

class Data {
    int *_dataP, *_maxDataP, *_minDataP;
    // Array with data values
    int *_data;

    DataOp *_undoP;
    // Undo-stack for data operations
    DataOp *_undoStack;

/* Hang Detection 1 collects effective data instruction. It collapses subsequent data instructions
 * that cancel each other out, e.g. DEC after INC, SHL after SHR. If at the end of the hang sample
 * period there are no effective instructions, it concludes that the program hangs.
 */
#ifdef HANG_DETECTION1
    DataOp *_effectiveP;
    // Stack with effective instruction in current hang sample period
    DataOp *_effective;
#endif

#ifdef HANG_DETECTION2
    // Array with data value deltas in current hange sample period
    int *_delta;

    int *_deltaP;
    int *_minDeltaP, *_minDeltaP0;
    int *_maxDeltaP, *_maxDeltaP0;
    int _significantValueChange;
    int _hangSamplePeriod;
#endif

public:
    Data(int size, int maxDataOps, int hangSamplePeriod);

    long getSize() { return _maxDataP - _minDataP + 1; }

    void resetHangDetection();
    bool isHangDetected();

    int val() { return *_dataP; }

    void inc();
    void dec();
    bool shr();
    bool shl();

    void undo(int num);

    void dump();
    void dumpStack();
};

#endif /* Data_h */
