//
//  Data.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

#include <stdint.h>
#include <vector>

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
    std::vector<int8_t> _undoStack;

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

    void delta(int delta);
    bool shift(int shift);

    size_t undoStackSize() const { return _undoStack.size(); }
    void undo(size_t targetSize);

    void dumpWithCursor(DataPointer cursor) const;
    void dump() const { dumpWithCursor(_dataP); };
    void dumpHangInfo() const;
};
