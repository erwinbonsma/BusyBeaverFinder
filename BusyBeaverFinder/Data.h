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

    // Delimits the data cells that are non-zero.
    DataPointer _minBoundP, _maxBoundP;

    // Array with data values
    int *_data;

    bool _undoEnabled;

    DataOp *_undoP = nullptr;
    DataOp *_maxUndoP = nullptr;
    // Undo-stack for data operations
    DataOp *_undoStack = nullptr;

    void updateBounds();

public:
    Data(int size);
    ~Data();

    void reset();

    void setEnableUndo(bool enable) { _undoEnabled = enable; }

    void setStackSize(int size);

    int getSize() const { return _size; }

    int* getDataBuffer() { return _data; }
    DataPointer getMinDataP() const { return _minDataP; }
    DataPointer getMaxDataP() const { return _maxDataP; }

    DataPointer getMinBoundP() const { return _minBoundP; }
    DataPointer getMaxBoundP() const { return _maxBoundP; }

    DataPointer getDataPointer() const { return _dataP; }
    int val() const { return *_dataP; }

    // Relatively slow but safe when dp+dpOffset might be out of bounds.
    int valueAt(DataPointer dp, int dpOffset) const;

    bool onlyZerosAhead(DataPointer dp, bool atRight) const;

    void inc();
    void dec();
    bool shr();
    bool shl();

    bool hasUndoCapacity() const { return _undoP < _maxUndoP; }
    int getUndoStackSize() const { return (int)(_undoP - _undoStack); }
    DataOp* getUndoStackPointer() const { return _undoP; }
    void undo(DataOp* _targetUndoP);

    void dump() const;
    void dumpStack() const;
    void dumpHangInfo() const;
};

#endif /* Data_h */
