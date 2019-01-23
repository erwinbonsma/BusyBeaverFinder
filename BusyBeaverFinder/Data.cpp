//
//  Data.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <stdio.h>
#include <iostream>

#include "Data.h"

Data::Data(int size, int maxDataOps, int hangSamplePeriod) {
    _data = new int[size];
    for (int i = 0; i < size; i++) {
        _data[i] = 0;
    }
    _dataP = &_data[size / 2];
    _minDataP = &_data[0];
    _maxDataP = &_data[size - 1];

    _undoStack = new DataOp[maxDataOps];
    _undoP = &_undoStack[0];

#ifdef HANG_DETECTION1
    _effective = new DataOp[hangSamplePeriod + 1];
    _effectiveP = &_effective[0];
    *(_effectiveP++) = DataOp::NONE; // Guard
#endif

#ifdef HANG_DETECTION2
    _hangSamplePeriod = hangSamplePeriod;
    _delta = new int[hangSamplePeriod * 2];
    // Init so that entire array is cleared by resetHangDetection()
    _minDeltaP = _delta;
    _maxDeltaP = _delta + hangSamplePeriod * 2 - 1;
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

#ifdef HANG_DETECTION2
    if (*_dataP == 0 || *_dataP == 1) {
        _significantValueChange = true;
    } else {
        (*_deltaP)++;
    }
#endif
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

#ifdef HANG_DETECTION2
    if (*_dataP == 0 || *_dataP == -1) {
        _significantValueChange = true;
    } else {
        (*_deltaP)--;
    }
#endif
}

bool Data::shr() {
    _dataP++;
    *(_undoP++) = DataOp::SHR;

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::SHL) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::SHR;
    }
#endif

#ifdef HANG_DETECTION2
    _deltaP++;
    if (_deltaP > _maxDeltaP) {
        _maxDeltaP = _deltaP;
    }
#endif

    return _dataP <= _maxDataP;
}

bool Data::shl() {
    _dataP--;
    *(_undoP++) = DataOp::SHL;

#ifdef HANG_DETECTION1
    if (*(_effectiveP - 1) == DataOp::SHR) {
        _effectiveP--;
    } else {
        *(_effectiveP++) = DataOp::SHL;
    }
#endif

#ifdef HANG_DETECTION2
    _deltaP--;
    if (_deltaP < _minDeltaP) {
        _minDeltaP = _deltaP;
    }
#endif

    return _dataP >= _minDataP;
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

#ifdef HANG_DETECTION2
    int *p = _minDeltaP;
    while (p <= _maxDeltaP) {
        *p = 0;
        p++;
    }

    _minDeltaP0 = _minDeltaP;
    _maxDeltaP0 = _maxDeltaP;
    _deltaP = &_delta[_hangSamplePeriod];
    _minDeltaP = _deltaP;
    _maxDeltaP = _deltaP;
    _significantValueChange = false;
#endif
}

bool Data::isHangDetected() {
#ifdef HANG_DETECTION1
    if (_effectiveP == &_effective[1]) {
        // No effective data instruction carried out
        return true;
    }
#endif

#ifdef HANG_DETECTION2
    if (
        // No hang if a data value became zero
        !_significantValueChange &&
        // or when new data cells were entered
        _minDeltaP >= _minDeltaP0 &&
        _maxDeltaP <= _maxDeltaP0
    ) {
        // Possible hang
        int *deltaP = _minDeltaP;
        int *dataP = _dataP + (deltaP - _deltaP);
        while (deltaP <= _maxDeltaP && ((*dataP) * (*deltaP)) >= 0) {
            deltaP++;
            dataP++;
        }
        if (deltaP > _maxDeltaP) {
            // All data cell changes were away from zero
            return true;
        }
    }
#endif

    return false;
}

void Data::dump() {
    // Find end
    int *max = _maxDataP;
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
