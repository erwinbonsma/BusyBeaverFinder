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
    int _size;

    // Delimits the data cells that have been visisted (since the last snapshot was taken)
    DataPointer _minVisitedP, _maxVisitedP;

    // Delimits the data cells that are non-zero.
    DataPointer _minBoundP, _maxBoundP;

    // Array with data values
    int *_data;

    bool _undoEnabled;

    DataOp *_undoP = nullptr;
    DataOp *_maxUndoP = nullptr;
    // Undo-stack for data operations
    DataOp *_undoStack = nullptr;

    bool _significantValueChange;

    void updateBounds();

public:
    Data(int size);
    ~Data();

    void reset();

    void setEnableUndo(bool enable) { _undoEnabled = enable; }

    void setStackSize(int size);

    int getSize() const { return _size; }

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

    // Relatively slow but safe when dp+dpOffset might be out of bounds.
    int valueAt(DataPointer dp, int dpOffset);

    void inc();
    void dec();
    bool shr();
    bool shl();

    bool hasUndoCapacity() { return _undoP < _maxUndoP; }
    int getUndoStackSize() { return (int)(_undoP - _undoStack); }
    DataOp* getUndoStackPointer() { return _undoP; }
    void undo(DataOp* _targetUndoP);

    void dump();
    void dumpStack();
    void dumpHangInfo();
};

#endif /* Data_h */
