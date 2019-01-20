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
    int *_data_p, *_data_p_max, *_data_p_min;
    int _data[dataSize];

    DataOp *_undo_p;
    DataOp _undo_stack[undoStackSize];

/* Hang Detection 1 collects effective data instruction. It collapses subsequent data instructions
 * that cancel each other out, e.g. DEC after INC, SHL after SHR. If at the end of the hang sample
 * period there are no effective instructions, it concludes that the program hangs.
 */
#ifdef HANG_DETECTION1
    DataOp *_effective_p;
    DataOp _effective[effectiveStackSize];
#endif

    void undo_last();

public:
    Data();

    void resetHangDetection();
    bool isHangDetected();

    int val() { return *_data_p; }

    void inc();
    void dec();
    bool shr();
    bool shl();

    void undo(int num);

    void dump();
    void dump_stack();
};

#endif /* Data_h */
