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

// Extra capacity beyond the requested maximum (as a single program block may add multiple items
// to the stack and ExhaustiveSearcher::run does not check capacity)
const int undoStackSentinelCapacity = 64;

Data::Data(int size) {
    _size = size;
    _data = new int[_size];

    _midDataP = &_data[_size / 2];
    _minDataP = &_data[0]; // Inclusive
    _maxDataP = &_data[_size - 1]; // Inclusive

    reset();
}

Data::~Data() {
    delete[] _data;

    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }
}

void Data::reset() {
    for (int i = _size; --i >= 0; ) {
        _data[i] = 0;
    }

    _dataP = _midDataP;

    _minBoundP = _midDataP;
    _maxBoundP = _minBoundP - 1; // Empty bounds

    _undoP = _undoStack;
}

void Data::setStackSize(int size) {
    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }

    _undoStack = new DataOp[size + undoStackSentinelCapacity];
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
    int index = (int)(dp - _minBoundP) + dpOffset;

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

void Data::inc() {
    (*_dataP)++;

    if (_undoEnabled) {
        *(_undoP++) = DataOp::INC;
    }

    updateBounds();
}

void Data::dec() {
    (*_dataP)--;

    if (_undoEnabled) {
        *(_undoP++) = DataOp::DEC;
    }

    updateBounds();
}

bool Data::shr() {
    _dataP++;

    if (_undoEnabled) {
        *(_undoP++) = DataOp::SHR;
    }

    return _dataP < _maxDataP;
}

bool Data::shl() {
    _dataP--;

    if (_undoEnabled) {
        *(_undoP++) = DataOp::SHL;
    }

    return _dataP > _minDataP;
}

void Data::undo(DataOp* _targetUndoP) {
    while (_undoP != _targetUndoP) {
        switch (*(--_undoP)) {
            case DataOp::INC: (*_dataP)--; updateBounds(); break;
            case DataOp::DEC: (*_dataP)++; updateBounds(); break;
            case DataOp::SHR: _dataP--; break;
            case DataOp::SHL: _dataP++; break;
            case DataOp::NONE: break;
        }
    }
}

void Data::dump() const {
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
        if (p == _dataP) {
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
    DataOp *p = &_undoStack[0];
    while (p < _undoP) {
        if (p != &_undoStack[0]) {
            std::cout << ",";
        }
        std::cout << (char)*p;
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
