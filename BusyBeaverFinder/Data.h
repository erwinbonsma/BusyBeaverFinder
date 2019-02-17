//
//  Data.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Data_h
#define Data_h

#include <stdio.h>

#include "Consts.h"
#include "Enums.h"

class Data {
    int *_dataP;
    int *_minDataP, *_midDataP, *_maxDataP;

    // Delimits the data cells that have been visisted (since the last snapshot was taken)
    int *_minVisitedP, *_maxVisitedP;

    // Delimits the data cells that are non-zero.
    int *_minBoundP, *_maxBoundP;

    // Array with data values
    int *_data;

    DataOp *_undoP = nullptr;
    // Undo-stack for data operations
    DataOp *_undoStack = nullptr;

    bool _significantValueChange;

    void updateBounds();

public:
    Data(int size);
    ~Data();

    void setStackSize(int size);
    void setHangSamplePeriod(int period);

    int getSize() { return (int)(_maxDataP - _minDataP + 1); }

    void resetHangDetection();

    void resetVisitedBounds();
    int* getMinVisitedP() { return _minVisitedP; }
    int* getMaxVisitedP() { return _maxVisitedP; }

    int* getDataBuffer() { return _data; }
    int* getMinDataP() { return _minDataP; }
    int* getMaxDataP() { return _maxDataP; }

    int* getMinBoundP() { return _minBoundP; }
    int* getMaxBoundP() { return _maxBoundP; }

    /* True if one or more values since last snapshot became zero, or moved away from zero.
     */
    bool significantValueChange() { return _significantValueChange; }

    int* getDataPointer() { return _dataP; }
    int val() { return *_dataP; }

    void inc();
    void dec();
    bool shr();
    bool shl();

    void undo(int num);

    void dump();
    void dumpStack();
    void dumpHangInfo();
};

#endif /* Data_h */
