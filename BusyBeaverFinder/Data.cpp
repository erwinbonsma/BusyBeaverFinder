//
//  Data.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <assert.h>
#include <stdio.h>
#include <iostream>

#include "Data.h"
#include "Utils.h"

// Extra capacity add both ends of the data buffer so that pointer-arithmetic for non-unit shifts
// is always valid.
const int dataSentinelBufferSize = 16;

enum class DataOp : unsigned char {
    INC = 0x00,
    DEC = 0x40,
    SHR = 0x80,
    SHL = 0xc0,
};

Data::Data(int size) {
    _size = size;
    _data = new int[_size + 2 * dataSentinelBufferSize];

    _midDataP = &_data[dataSentinelBufferSize + _size / 2];
    _minDataP = &_data[dataSentinelBufferSize]; // Inclusive
    _maxDataP = &_data[dataSentinelBufferSize + _size - 1]; // Inclusive

    reset();
}

Data::~Data() {
    delete[] _data;

    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }
}

void Data::reset() {
    for (int i = _size + 2 * dataSentinelBufferSize; --i >= 0; ) {
        _data[i] = 0;
    }

    _dataP = _midDataP;

    _minBoundP = _midDataP;
    _maxBoundP = _minBoundP - 1; // Empty bounds

    _undoP = _undoStack;
    _undoEnabled = true;
}

void Data::setStackSize(int size) {
    if (_undoStack != nullptr) {
        if (_maxUndoP - _undoP == size) {
            // Nothing needs doing. Stack size is unchanged
            return;
        }

        delete[] _undoStack;
    }

    _undoStack = new UndoOp[size];
    _undoP = _undoStack;
    _maxUndoP = _undoStack + size;
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

void Data::inc(unsigned char delta) {
    (*_dataP) += delta;

    if (_undoEnabled) {
        *(_undoP++) = delta | (unsigned char)DataOp::INC;
    }

    updateBounds();
}

void Data::dec(unsigned char delta) {
    (*_dataP) -= delta;

    if (_undoEnabled) {
        *(_undoP++) = delta | (unsigned char)DataOp::DEC;
    }

    updateBounds();
}

bool Data::shr(unsigned char shift) {
    _dataP += shift;

    if (_undoEnabled) {
        *(_undoP++) = shift | (unsigned char)DataOp::SHR;
    }

    return _dataP < _maxDataP;
}

bool Data::shl(unsigned char shift) {
    _dataP -= shift;

    if (_undoEnabled) {
        *(_undoP++) = shift | (unsigned char)DataOp::SHL;
    }

    return _dataP > _minDataP;
}

void Data::undo(const UndoOp* _targetUndoP) {
    while (_undoP != _targetUndoP) {
        UndoOp undo = *(--_undoP);
        switch (undo & 0xc0) {
            case (int)DataOp::INC: (*_dataP) -= undo & 0x3f; updateBounds(); break;
            case (int)DataOp::DEC: (*_dataP) += undo & 0x3f; updateBounds(); break;
            case (int)DataOp::SHR: _dataP -= undo & 0x3f; break;
            case (int)DataOp::SHL: _dataP += undo & 0x3f; break;
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
    std::cout << std::endl;
}

void Data::dumpStack() const {
    UndoOp *p = &_undoStack[0];
    while (p < _undoP) {
        if (p != &_undoStack[0]) {
            std::cout << ",";
        }
        std::cout << *p;
        p++;
    }
}

void Data::dumpHangInfo() const {
    std::cout << "DATA: min = " << (_minBoundP - _data)
    << ", p = " << (_dataP - _data)
    << ", max = " << (_maxBoundP - _data)
    << std::endl;
    dumpDataBuffer(_data, _dataP, getSize());
}
