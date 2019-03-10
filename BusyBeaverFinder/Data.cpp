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

Data::Data(int size) {
    _data = new int[size];

    for (int i = 0; i < size; i++) {
        _data[i] = 0;
    }

    _midDataP = &_data[size / 2];
    _minDataP = &_data[0]; // Inclusive
    _maxDataP = &_data[size - 1]; // Inclusive
    _dataP = _midDataP;

    _minBoundP = _midDataP;
    _maxBoundP = _minBoundP - 1; // Empty bounds
}

Data::~Data() {
    delete[] _data;

    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }
}

void Data::setStackSize(int size) {
    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }

    _undoStack = new DataOp[size];
    _undoP = &_undoStack[0];
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

void Data::setHangSamplePeriod(int period) {
}

void Data::inc() {
    (*_dataP)++;
    *(_undoP++) = DataOp::INC;

    updateBounds();

    if (*_dataP == 0 || *_dataP == 1) {
        _significantValueChange = true;
    }
}

void Data::dec() {
    (*_dataP)--;
    *(_undoP++) = DataOp::DEC;

    updateBounds();

    if (*_dataP == 0 || *_dataP == -1) {
        _significantValueChange = true;
    }
}

bool Data::shr() {
    _dataP++;
    *(_undoP++) = DataOp::SHR;

    if (_dataP > _maxVisitedP) {
        _maxVisitedP = _dataP;
    }

    return _dataP < _maxDataP;
}

bool Data::shl() {
    _dataP--;
    *(_undoP++) = DataOp::SHL;

    if (_dataP < _minVisitedP) {
        _minVisitedP = _dataP;
    }

    return _dataP > _minDataP;
}

void Data::undo(int num) {
    while (--num >= 0) {
        switch (*(--_undoP)) {
            case DataOp::INC: (*_dataP)--; updateBounds(); break;
            case DataOp::DEC: (*_dataP)++; updateBounds(); break;
            case DataOp::SHR: _dataP--; break;
            case DataOp::SHL: _dataP++; break;
            case DataOp::NONE: break;
        }
    }
}

void Data::resetHangDetection() {
    resetVisitedBounds();

    _significantValueChange = false;
}

void Data::resetVisitedBounds() {
    _minVisitedP = _dataP;
    _maxVisitedP = _dataP;
}

void Data::dump() {
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

void Data::dumpStack() {
    DataOp *p = &_undoStack[0];
    while (p < _undoP) {
        if (p != &_undoStack[0]) {
            std::cout << ",";
        }
        std::cout << (char)*p;
        p++;
    }
}

void Data::dumpHangInfo() {
    std::cout << "DATA: min = " << (_minBoundP - _data)
    << ", p = " << (_dataP - _data)
    << ", max = " << (_maxBoundP - _data)
    << std::endl;
    dumpDataBuffer(_data, _dataP, getSize());
}
