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

    DataOp *_effective_p;
    DataOp _effective[effectiveStackSize];

    void undo_last();

public:
    Data();

    void resetHangDetection() { _effective_p = &_effective[1]; }
    bool isHangDetected() { return _effective_p == &_effective[1]; }

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
