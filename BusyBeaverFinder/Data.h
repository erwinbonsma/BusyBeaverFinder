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
    int _data[dataSize];

    DataOp *_undoP;
    DataOp _undoStack[undoStackSize];

/* Hang Detection 1 collects effective data instruction. It collapses subsequent data instructions
 * that cancel each other out, e.g. DEC after INC, SHL after SHR. If at the end of the hang sample
 * period there are no effective instructions, it concludes that the program hangs.
 */
#ifdef HANG_DETECTION1
    DataOp *_effectiveP;
    DataOp _effective[effectiveStackSize];
#endif

#ifdef HANG_DETECTION2
    int _delta[hangDeltaSize];

    int *_deltaP;
    int *_minNonZeroDeltaP, *_minNonZeroDeltaP0;
    int *_maxNonZeroDeltaP, *_maxNonZeroDeltaP0;
    int _significantValueChange;
#endif

public:
    Data();

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
