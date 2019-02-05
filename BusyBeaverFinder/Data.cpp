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

    _snapShotA.buf = new int[size];
    _snapShotB.buf = new int[size];

    for (int i = 0; i < size; i++) {
        _data[i] = 0;
        _snapShotA.buf[i] = 0;
        _snapShotB.buf[i] = 0;
    }

    _midDataP = &_data[size / 2];
    _minDataP = &_data[0]; // Inclusive
    _maxDataP = &_data[size - 1]; // Inclusive
    _dataP = _midDataP;
}

Data::~Data() {
    delete[] _data;
    delete[] _snapShotA.buf;
    delete[] _snapShotB.buf;

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

    _oldSnapShotP = nullptr;
    _newSnapShotP = nullptr;

    _minVisitedP = _dataP;
    _maxVisitedP = _dataP;

    _significantValueChange = false;
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

void Data::captureSnapShot() {
    if (_newSnapShotP == nullptr) {
//        std::cout << "Snapshot 1" << std::endl;
        _newSnapShotP = &_snapShotA;
    }
    else if (_oldSnapShotP == nullptr) {
//        std::cout << "Snapshot 2" << std::endl;
        _oldSnapShotP = _newSnapShotP;
        _newSnapShotP = &_snapShotB;
    }
    else {
//        std::cout << "Snapshot N" << std::endl;
        SnapShot *tmp = _newSnapShotP;
        _newSnapShotP = _oldSnapShotP;
        _oldSnapShotP = tmp;
    }

    // TODO: A full copy is not needed when old snapshot is reused. In this case, a smart partial
    // update suffices.
    int *buf = _newSnapShotP->buf;
    memcpy(buf, _data, sizeof(int) * getSize());

    _newSnapShotP->dataP =_dataP;
    _newSnapShotP->minVisitedP = _minVisitedP;
    _newSnapShotP->maxVisitedP = _maxVisitedP;

    _minVisitedP = _dataP;
    _maxVisitedP = _dataP;
}

// Checks if a change in data value is impactful. An impactful change is one that, when carried out
// repeatedly, will impact a TURN evaluation. I.e. a data value moved closer to zero (or its value
// was zero and not anymore). In the macro, x is the old value and y the new value.
#define IMPACTFUL_CHANGE(x, y) ((x <= 0 && y > x) || (x >= 0 && y < x))

SnapShotComparison Data::compareToSnapShot() {
    SnapShotComparison result = SnapShotComparison::UNCHANGED;

    int *p1 = _minVisitedP;
    int *p2 = _newSnapShotP->buf + (p1 - _data);
    do {
        if (*p1 != *p2) {
            if (IMPACTFUL_CHANGE(*p2, *p1)) {
                return SnapShotComparison::IMPACTFUL;
            } else {
                result = SnapShotComparison::DIVERGING;
            }
        }
        p1++;
        p2++;
    } while (p1 <= _maxVisitedP);

    return result;
}

bool Data::areSnapShotDeltasAreIdentical() {
    long _deltaNew = _maxVisitedP - _minVisitedP;
    long _deltaOld = _newSnapShotP->maxVisitedP - _newSnapShotP->minVisitedP;

    if (_deltaNew != _deltaOld) {
        // The number of cells visited differ
        return false;
    }

    int *oldBeforeP = _oldSnapShotP->buf + (_newSnapShotP->minVisitedP - _data);
    int *oldAfterP = _newSnapShotP->buf + (_newSnapShotP->minVisitedP - _data);
    int *newBeforeP = _newSnapShotP->buf + (_minVisitedP - _data);
    int *newAfterP = _data + (_minVisitedP - _data); // Expanded for clarity

    do {
//        std::cout
//        << "OLD: " << *oldBeforeP << " => " << *oldAfterP << std::endl
//        << "NEW: " << *newBeforeP << " => " << *newAfterP << std::endl;
        if (
            *oldBeforeP != *newBeforeP ||
            *oldAfterP != *newAfterP
        ) {
            return false;
        }
        oldBeforeP++;
        newBeforeP++;
        oldAfterP++;
        newAfterP++;
    } while (newAfterP <= _maxVisitedP);

    long shift = _dataP - _newSnapShotP->dataP;
    if (shift > 0) {
        // Check that the newly visited values were all zeros
        newBeforeP -= shift;
        while (shift > 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++;
            shift--;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        while (newAfterP <= _maxDataP) {
            if (*newAfterP != 0) {
                return false;
            }
            newAfterP++;
        }
    } else {
        // Check that the newly visited values were all zeros
        newBeforeP = _newSnapShotP->buf + (_minVisitedP - _data);
        while (shift < 0) {
            if (*newBeforeP != 0) {
                return false;
            }
            newBeforeP++; // Note: The increase is intentional
            shift++;
        }

        // Check that there are only zeros ahead. Other values may break the repetitive behavior
        newAfterP = _minVisitedP - 1;
        while (newAfterP >= _minDataP) {
            if (*newAfterP != 0) {
                return false;
            }
            newAfterP--;
        }
    }

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
#ifdef HANG_DETECTION2B
    << " 2B"
#endif
#ifndef HANG_DETECTION1
#ifndef HANG_DETECTION2B
    << " None"
#endif
#endif
    << std::endl;
}

void Data::dumpDataBuffer(int* buf, int* dataP) {
    int size = (int)getSize();
    for (int i = 0; i < size; i++) {
        if (&buf[i] == dataP) {
            std::cout << "[";
        }
        std::cout << buf[i];
        if (&buf[i] == dataP) {
            std::cout << "] ";
        } else {
            std::cout << " ";
        }

    }
    std::cout << std::endl;
}

void Data::dumpHangInfo() {
    std::cout << "DATA: min = " << (_minVisitedP - _data)
    << ", p = " << (_dataP - _data)
    << ", max = " << (_maxVisitedP - _data)
    << std::endl;
    dumpDataBuffer(_data, _dataP);

    if (_newSnapShotP != nullptr) {
        std::cout << "SNAP1: min = " << (_newSnapShotP->minVisitedP - _data)
        << ", p = " << (_newSnapShotP->dataP - _data)
        << ", max = " << (_newSnapShotP->maxVisitedP - _data)
        << std::endl;
        dumpDataBuffer(_newSnapShotP->buf, _newSnapShotP->buf + (_newSnapShotP->dataP - _data));
    }

    if (_oldSnapShotP != nullptr) {
        std::cout << "SNAP2: min = " << (_oldSnapShotP->minVisitedP - _data)
        << ", p = " << (_oldSnapShotP->dataP - _data)
        << ", max = " << (_oldSnapShotP->maxVisitedP - _data)
        << std::endl;
        dumpDataBuffer(_oldSnapShotP->buf, _oldSnapShotP->buf + (_oldSnapShotP->dataP - _data));
    }
}
