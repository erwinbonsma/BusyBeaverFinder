//
//  Data.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//
#pragma once

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

    void updateBounds();

public:
    Data(int size);
    ~Data();

    void reset();

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

    void dumpWithCursor(DataPointer cursor) const;
    void dump() const { dumpWithCursor(_dataP); };
    void dumpHangInfo() const;
};
