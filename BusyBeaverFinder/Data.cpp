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

Data::Data() {
    for (int i = 0; i < dataSize; i++) {
        _data[i] = 0;
    }
    _dataP = &_data[dataSize / 2];
    _minDataP = &_data[0];
    _maxDataP = &_data[dataSize - 1];

    _undoP = &_undoStack[0];


#ifdef HANG_DETECTION1
    _effectiveP = &_effective[0];
    *(_effectiveP++) = DataOp::NONE; // Guard
#endif

#ifdef HANG_DETECTION2
    _minNonZeroDeltaP = 0;
    _maxNonZeroDeltaP = 0;
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
    if (*_dataP != 0 && _deltaP > _maxNonZeroDeltaP) {
        _maxNonZeroDeltaP = _deltaP;
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
    if (*_dataP != 0 && _deltaP < _minNonZeroDeltaP) {
        _minNonZeroDeltaP = _deltaP;
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
    for (int i = hangDeltaSize; --i >= 0; ) {
        _delta[i] = 0;
    }

    _minNonZeroDeltaP0 = _minNonZeroDeltaP;
    _maxNonZeroDeltaP0 = _maxNonZeroDeltaP;
    _deltaP = &_delta[hangDeltaSize / 2];
    _minNonZeroDeltaP = _deltaP;
    _maxNonZeroDeltaP = _deltaP;
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
//    int *deltaP = _minNonZeroDeltaP;
//    int *dataP = _dataP + (deltaP - _deltaP);
//    std::cout
//        << "Offset = " << (dataP - &_data[dataSize / 2])
//        << ", Width = " << (_maxNonZeroDeltaP - _minNonZeroDeltaP) << "\n";
//    while (deltaP <= _maxNonZeroDeltaP) {
//        std::cout << *dataP << "("
//        << *deltaP << ") ";
//        deltaP++;
//        dataP++;
//    }
//    std::cout << "\n";

    if (
        // No hang if a data value became zero
        !_significantValueChange &&
        // or when new non-zero data cells were entered
        _minNonZeroDeltaP >= _minNonZeroDeltaP0 &&
        _maxNonZeroDeltaP <= _maxNonZeroDeltaP0
    ) {
        // Possible hang
        int *deltaP = _minNonZeroDeltaP;
        int *dataP = _dataP + (deltaP - _deltaP);
        while (deltaP <= _maxNonZeroDeltaP && ((*dataP) * (*deltaP)) >= 0) {
            deltaP++;
            dataP++;
        }
        if (deltaP > _maxNonZeroDeltaP) {

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
    std::cout << "\n";
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
