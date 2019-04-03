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
#include "Types.h"

class Data {
    DataPointer _dataP;
    DataPointer _minDataP, _midDataP, _maxDataP;

    // Delimits the data cells that have been visisted (since the last snapshot was taken)
    DataPointer _minVisitedP, _maxVisitedP;

    // Delimits the data cells that are non-zero.
    DataPointer _minBoundP, _maxBoundP;

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

    int getSize() { return (int)(_maxDataP - _minDataP + 1); }

    void resetHangDetection();

    void resetVisitedBounds();
    DataPointer getMinVisitedP() { return _minVisitedP; }
    DataPointer getMaxVisitedP() { return _maxVisitedP; }

    int* getDataBuffer() { return _data; }
    DataPointer getMinDataP() { return _minDataP; }
    DataPointer getMaxDataP() { return _maxDataP; }

    DataPointer getMinBoundP() { return _minBoundP; }
    DataPointer getMaxBoundP() { return _maxBoundP; }

    /* True if one or more values since last snapshot became zero, or moved away from zero.
     */
    bool significantValueChange() { return _significantValueChange; }

    DataPointer getDataPointer() { return _dataP; }
    int val() { return *_dataP; }

    void inc();
    void dec();
    bool shr();
    bool shl();

    DataOp* getUndoStackPointer() { return _undoP; }
    void undo(DataOp* _targetUndoP);

    void dump();
    void dumpStack();
    void dumpHangInfo();
};

#endif /* Data_h */
