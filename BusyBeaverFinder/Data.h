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

    DataOp *_undoP = nullptr;
    // Undo-stack for data operations
    DataOp *_undoStack = nullptr;

/* Hang Detection 1 collects effective data instruction. It collapses subsequent data instructions
 * that cancel each other out, e.g. DEC after INC, SHL after SHR. If at the end of the hang sample
 * period there are no effective instructions, it concludes that the program hangs.
 */
#ifdef HANG_DETECTION1
    DataOp *_effectiveP = nullptr;
    // Stack with effective instruction in current hang sample period
    DataOp *_effective = nullptr;
#endif

#ifdef HANG_DETECTION2
    // Array with data value deltas in current hange sample period
    int *_delta = nullptr;

    int *_deltaP = nullptr;
    int *_minDeltaP = nullptr, *_minDeltaP0 = nullptr;
    int *_maxDeltaP = nullptr, *_maxDeltaP0 = nullptr;
    int _significantValueChange;
    int _hangSamplePeriod;
#endif

public:
    Data(int size);

    void setStackSize(int size);
    void setHangSamplePeriod(int period);

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
    void dumpSettings();
};

#endif /* Data_h */
