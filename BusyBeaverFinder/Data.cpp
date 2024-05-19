//
//  Data.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "Data.h"

#include <assert.h>
#include <iostream>

#include "Utils.h"

// Extra capacity add both ends of the data buffer so that pointer-arithmetic for non-unit shifts
// is always valid.
const int dataSentinelBufferSize = 16;

enum class DataOp : int8_t {
    DELTA = 0x00,
    SHIFT = 0x01,
};

Data::Data(int size) {
    _size = size;
    _data.reserve(_size + 2 * dataSentinelBufferSize);

    _midDataP = &_data[dataSentinelBufferSize + _size / 2];
    _minDataP = &_data[dataSentinelBufferSize]; // Inclusive
    _maxDataP = &_data[dataSentinelBufferSize + _size - 1]; // Inclusive

    reset();
}

void Data::reset() {
    for (int i = _size + 2 * dataSentinelBufferSize; --i >= 0; ) {
        _data[i] = 0;
    }

    _dataP = _midDataP;

    _minBoundP = _midDataP;
    _maxBoundP = _minBoundP - 1; // Empty bounds

    _undoStack.clear();
    _undoEnabled = true;
}

void Data::updateBounds() {
    if (*_dataP == 0) {
        if (_dataP == _minBoundP) {
            do {
                _minBoundP++;
            } while (*_minBoundP == 0 && _minBoundP <= _maxBoundP);
        }
        else if (_dataP == _maxBoundP) {
            do {
                _maxBoundP--;
            } while (*_maxBoundP == 0 && _maxBoundP >= _minBoundP);
        }
    } else {
        if (_minBoundP > _maxBoundP) {
            _minBoundP = _dataP;
            _maxBoundP = _dataP;
        }
        else if (_dataP < _minBoundP) {
            _minBoundP = _dataP;
        }
        else if (_dataP > _maxBoundP) {
            _maxBoundP = _dataP;
        }
    }

    assert((*_minBoundP && *_maxBoundP) || (_maxBoundP < _minBoundP));
}

int Data::valueAt(DataPointer dp, int dpOffset) const {
    int index = (int)(dp - _minDataP) + dpOffset;

    return (index >= 0 && index < _size) ? *(_minDataP + index) : 0;
}

// Note: Analysis skips the value at DP. It only considers the values beyond that.
bool Data::onlyZerosAhead(DataPointer dp, bool atRight) const {
    if (atRight) {
        if (dp >= _maxBoundP) {
            return true;
        }

        while (true) {
            dp++;
            if (*dp) { return false; }
            if (dp == _maxBoundP) { break; }
        }
    } else {
        if (dp <= _minBoundP) {
            return true;
        }

        while (true) {
            dp--;
            if (*dp) { return false; }
            if (dp == _minBoundP) { break; }
        }
    }

    return true;
}

void Data::delta(int delta) {
    (*_dataP) += delta;
    if (_undoEnabled) _undoStack.push_back(delta << 1 | (int8_t)DataOp::DELTA);
    updateBounds();
}

bool Data::shift(int shift) {
    _dataP += shift;
    if (_undoEnabled) _undoStack.push_back(shift << 1 | (int8_t)DataOp::SHIFT);
    return _dataP > _minDataP && _dataP < _maxDataP;
}

void Data::undo(size_t targetSize) {
    assert(targetSize <= _undoStack.size());

    while (_undoStack.size() > targetSize) {
        int8_t undo = _undoStack.back();
        _undoStack.pop_back();
        if (undo & (int8_t)DataOp::SHIFT) {
            _dataP -= (undo >> 1);
        } else {
            (*_dataP) -= (undo >> 1);
            updateBounds();
        }
    }
}

void Data::dumpWithCursor(DataPointer cursor) const {
    // Find end
    int *max = _maxDataP - 1;
    while (max > _dataP && *max == 0) {
        max--;
    }
    // Find start
    int *p = _minDataP;
    while (p < _dataP && *p == 0) {
        p++;
    }

    std::cout << "Data: ";
    while (1) {
        if (p == _minBoundP) {
            std::cout << "<< ";
        }
        if (p == cursor) {
            std::cout << "[" << *p << "]";
        } else {
            std::cout << *p;
        }
        if (p == _maxBoundP) {
            std::cout << " >>";
        }
        if (p < max) {
            p++;
            std::cout << " ";
        } else {
            break;
        }
    }
    std::cout << ", dp = " << (cursor - _midDataP) << std::endl;
}
