//
//  Data.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

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
}

Data::~Data() {
    delete[] _data;

    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }

#ifdef HANG_DETECTION1
    if (_effective != nullptr) {
        delete[] _effective;
    }
#endif
}

void Data::setStackSize(int size) {
    if (_undoStack != nullptr) {
        delete[] _undoStack;
    }

    _undoStack = new DataOp[size];
    _undoP = &_undoStack[0];
}

void Data::setHangSamplePeriod(int period) {
#ifdef HANG_DETECTION1
    if (_effective != nullptr) {
        delete[] _effective;
    }
    _effective = new DataOp[period + 1];
    _effectiveP = &_effective[0];
    *(_effectiveP++) = DataOp::NONE; // Guard
#endif
}

void Data::inc() {
    (*_dataP)++;
    *(_undoP++) = DataOp::INC;

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::DEC) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::INC;
    }
#endif

    if (*_dataP == 0 || *_dataP == 1) {
        _significantValueChange = true;
    }
}

void Data::dec() {
    (*_dataP)--;
    *(_undoP++) = DataOp::DEC;

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::INC) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::DEC;
    }
#endif

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

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::SHL) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::SHR;
    }
#endif

    return _dataP < _maxDataP;
}

bool Data::shl() {
    _dataP--;
    *(_undoP++) = DataOp::SHL;

    if (_dataP < _minVisitedP) {
        _minVisitedP = _dataP;
    }

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::SHR) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::SHL;
    }
#endif

    return _dataP > _minDataP;
}

void Data::undo(int num) {
    while (--num >= 0) {
        switch (*(--_undoP)) {
            case DataOp::INC: (*_dataP)--; break;
            case DataOp::DEC: (*_dataP)++; break;
            case DataOp::SHR: _dataP--; break;
            case DataOp::SHL: _dataP++; break;
            case DataOp::NONE: break;
        }
    }
}

void Data::resetHangDetection() {
#ifdef HANG_DETECTION1
    _effectiveP = &_effective[1];
#endif

    resetVisitedBounds();

    _significantValueChange = false;
}

void Data::resetVisitedBounds() {
    _minVisitedP = _dataP;
    _maxVisitedP = _dataP;
}

bool Data::effectiveDataOperations() {
#ifdef HANG_DETECTION1
    if (_effectiveP == &_effective[1]) {
        // No effective data instruction carried out
        return false;
    }
#endif
    return true;
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
        if (p == _dataP) {
            std::cout << "[" << *p << "]";
        } else {
            std::cout << *p;
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

void Data::dumpSettings() {
    std::cout
    << "Enabled hang detections:"
#ifdef HANG_DETECTION1
    << " 1"
#endif
#ifndef HANG_DETECTION1
    << " None"
#endif
    << std::endl;
}

void Data::dumpHangInfo() {
    std::cout << "DATA: min = " << (_minVisitedP - _data)
    << ", p = " << (_dataP - _data)
    << ", max = " << (_maxVisitedP - _data)
    << std::endl;
    dumpDataBuffer(_data, _dataP, getSize());
}
