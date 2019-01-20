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
    _data_p = &_data[dataSize / 2];
    _data_p_min = &_data[0];
    _data_p_max = &_data[dataSize - 1];

    _undo_p = &_undo_stack[0];


#ifdef HANG_DETECTION1
    _effective_p = &_effective[0];
    *(_effective_p++) = DataOp::NONE; // Guard
#endif
}

void Data::inc() {
    (*_data_p)++;
    *(_undo_p++) = DataOp::INC;

#ifdef HANG_DETECTION1
    if (*(_effective_p - 1) == DataOp::DEC) {
        _effective_p--;
    } else {
        *(_effective_p++) = DataOp::INC;
    }
#endif
}

void Data::dec() {
    (*_data_p)--;
    *(_undo_p++) = DataOp::DEC;

#ifdef HANG_DETECTION1
    if (*(_effective_p - 1) == DataOp::INC) {
        _effective_p--;
    } else {
        *(_effective_p++) = DataOp::DEC;
    }
#endif
}

bool Data::shr() {
    _data_p++;
    *(_undo_p++) = DataOp::SHR;

#ifdef HANG_DETECTION1
    if (*(_effective_p - 1) == DataOp::SHL) {
        _effective_p--;
    } else {
        *(_effective_p++) = DataOp::SHR;
    }
#endif
    return _data_p <= _data_p_max;
}

bool Data::shl() {
    _data_p--;
    *(_undo_p++) = DataOp::SHL;

#ifdef HANG_DETECTION1
    if (*(_effective_p - 1) == DataOp::SHR) {
        _effective_p--;
    } else {
        *(_effective_p++) = DataOp::SHL;
    }
#endif
    return _data_p >= _data_p_min;
}

void Data::undo(int num) {
    while (--num >= 0) {
        switch (*(--_undo_p)) {
            case DataOp::INC: (*_data_p)--; break;
            case DataOp::DEC: (*_data_p)++; break;
            case DataOp::SHR: _data_p--; break;
            case DataOp::SHL: _data_p++; break;
            case DataOp::NONE: break;
        }
    }
}

void Data::resetHangDetection() {
#ifdef HANG_DETECTION1
    _effective_p = &_effective[1];
#endif
}
bool Data::isHangDetected() {
#ifdef HANG_DETECTION1
    if (_effective_p == &_effective[1]) {
        // No effective data instruction carried out
        return true;
    }
#endif
    return false;
}

void Data::dump() {
    // Find end
    int *max = _data_p_max;
    while (max > _data_p && *max == 0) {
        max--;
    }
    // Find start
    int *p = _data_p_min;
    while (p < _data_p && *p == 0) {
        p++;
    }

    while (1) {
        if (p == _data_p) {
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

void Data::dump_stack() {
    DataOp *p = &_undo_stack[0];
    while (p < _undo_p) {
        if (p != &_undo_stack[0]) {
            std::cout << ",";
        }
        std::cout << (char)*p;
        p++;
    }
}
