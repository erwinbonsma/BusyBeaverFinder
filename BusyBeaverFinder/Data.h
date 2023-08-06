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
#include <stdint.h>

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

    UndoOp *_undoP = nullptr;
    UndoOp *_maxUndoP = nullptr;
    // Undo-stack for data operations
    UndoOp *_undoStack = nullptr;

    void updateBounds();

public:
    Data(int size);
    ~Data();

    void reset();

    void setEnableUndo(bool enable) { _undoEnabled = enable; }
    bool undoEnabled() const { return _undoEnabled; }

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

    void inc(uint8_t delta);
    void dec(uint8_t delta);
    bool shr(uint8_t shift);
    bool shl(uint8_t shift);

    void inc() { inc(1); };
    void dec() { dec(1); };
    bool shr() { return shr(1); };
    bool shl() { return shl(1); };

    bool hasUndoCapacity() const { return _undoP < _maxUndoP; }
    int getUndoStackSize() const { return (int)(_undoP - _undoStack); }
    const UndoOp* getUndoStackPointer() const { return _undoP; }
    void undo(const UndoOp* _targetUndoP);

    void dumpWithCursor(DataPointer cursor) const;
    void dump() const { dumpWithCursor(_dataP); };
    void dumpStack() const;
    void dumpHangInfo() const;
};

#endif /* Data_h */
