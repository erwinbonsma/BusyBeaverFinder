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

Data::Data(int size) {
    _data = new int[size];
#ifdef HANG_DETECTION3
    _snapShotData = new int[size];
#endif

    for (int i = 0; i < size; i++) {
        _data[i] = 0;
#ifdef HANG_DETECTION3
        _snapShotData[i] = 0; // Not strictly needed, but there's no harm
#endif
    }

    _midDataP = &_data[size / 2];
    _minDataP = &_data[0]; // Inclusive
    _maxDataP = &_data[size]; // Exclusive
    _dataP = _midDataP;
}

void Data::setStackSize(int size) {
    if (_undoStack != nullptr) {
        delete _undoStack;
    }

    _undoStack = new DataOp[size];
    _undoP = &_undoStack[0];
}

void Data::setHangSamplePeriod(int period) {
#ifdef HANG_DETECTION1
    if (_effective != nullptr) {
        delete _effective;
    }
    _effective = new DataOp[period + 1];
    _effectiveP = &_effective[0];
    *(_effectiveP++) = DataOp::NONE; // Guard
#endif

#ifdef HANG_DETECTION2
    _hangSamplePeriod = period;
    _delta = new int[period * 2];
    // Init so that entire array is cleared by resetHangDetection()
    _minDeltaP = _delta;
    _maxDeltaP = _delta + period * 2 - 1;
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

    return _dataP < _maxDataP;
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

    _prevMinDataP = _dataP + (_minDeltaP - _deltaP);
    _prevMaxDataP = _dataP + (_maxDeltaP - _deltaP);
    _prevMove = (int)(_deltaP - &_delta[_hangSamplePeriod]);
    _deltaP = &_delta[_hangSamplePeriod];
    _minDeltaP = _deltaP;
    _maxDeltaP = _deltaP;
    _significantValueChange = false;
#endif
}

bool Data::isHangDetected() {
    bool hangDetected = false;
#ifdef HANG_DETECTION1
    if (_effectiveP == &_effective[1]) {
        // No effective data instruction carried out
        hangDetected = true;
    }
#endif

#ifdef HANG_DETECTION2
    if (!hangDetected) {
        // Range of data cells entered in the current sample period
        int *minDataP = _dataP + (_minDeltaP - _deltaP);
        int *maxDataP = _dataP + (_maxDeltaP - _deltaP);
        int move = (int)(_deltaP - &_delta[_hangSamplePeriod]);
        if (
            // No hang if a data value became zero
            !_significantValueChange &&
            // or when new data cells were entered
            minDataP >= _prevMinDataP &&
            maxDataP <= _prevMaxDataP &&
            // or when the data pointer moved (significantly) in a different direction
            !(move < -4 && _prevMove > 4) &&
            !(move > 4 && _prevMove < -4)
        ) {
            // Possible hang
            int *deltaP = _minDeltaP;
            int *dataP = minDataP;
            while (deltaP <= _maxDeltaP && ((*dataP) * (*deltaP)) >= 0) {
                deltaP++;
                dataP++;
            }
            if (deltaP > _maxDeltaP) {
                // All data cell changes were away from zero
                hangDetected = true;
            }
        }
    }
#endif

    resetHangDetection();

    return hangDetected;
}

#ifdef HANG_DETECTION3
void Data::captureSnapShot() {
    memcpy(_snapShotData, _data, sizeof(int) * getSize());
}

// Checks if a change in data value is impactful. An impactful change is one that, when carried out
// repeatedly, will impact a TURN evaluation. I.e. a data value moved closer to zero (or its value
// was zero and not anymore). In the macro, x is the old value and y the new value.
#define IMPACTFUL_CHANGE(x, y) ((x <= 0 && y > x) || (x >= 0 && y < x))

SnapShotComparison Data::compareToSnapShot() {
    SnapShotComparison result = SnapShotComparison::UNCHANGED;

    // Instead of comparing from start of the tape towards the end, comparison starts in the
    // middle and moves outwards in both directions. This way, it first considers the values that
    // most frequently are changed so that comparison ends more quickly.
    int *dataP1 = _midDataP;
    int *snapP1 = _snapShotData + (dataP1 - _minDataP);
    int *dataP2 = dataP1 - 1;
    int *snapP2 = snapP1 - 1;
    do {
        if (*dataP1 != *snapP1) {
            if (IMPACTFUL_CHANGE(*dataP1, *snapP1)) {
                return SnapShotComparison::IMPACTFUL;
            } else {
                result = SnapShotComparison::DIVERGING;
            }
        }
        if (*dataP2 != *snapP2) {
            if (IMPACTFUL_CHANGE(*dataP2, *snapP2)) {
                return SnapShotComparison::IMPACTFUL;
            } else {
                result = SnapShotComparison::DIVERGING;
            }
        }
        dataP1++;
        snapP1++;
        dataP2--;
        snapP2--;
    } while (dataP1 < _maxDataP);

    return result;
}
#endif

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
#ifdef HANG_DETECTION2
    << " 2"
#endif
#ifdef HANG_DETECTION3
    << " 3"
#endif
#ifndef HANG_DETECTION1
#ifndef HANG_DETECTION2
#ifndef HANG_DETECTION3
    << " None"
#endif
#endif
#endif
    << std::endl;
}


void Data::dumpHangInfo() {
#ifdef HANG_DETECTION2
    int *minDataP = _dataP + (_minDeltaP - _deltaP);
    int *maxDataP = _dataP + (_maxDeltaP - _deltaP);
    int move = (int)(_deltaP - &_delta[_hangSamplePeriod]);

    std::cout
    << "Prev :" << (_prevMinDataP - _minDataP) << " - " << (_prevMaxDataP - _minDataP)
    << ", delta = " << _prevMove
    << std::endl
    << "Now  :" << (minDataP - _minDataP) << " - " << (maxDataP - _minDataP)
    << ", delta = " << move
    << std::endl;
#endif
}
