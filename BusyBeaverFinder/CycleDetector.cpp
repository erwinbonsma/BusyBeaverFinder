//
//  CycleDetector.c
//  BusyBeaverFinder
//
//  Created by Erwin on 07/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <iostream>

#include "CycleDetector.h"

#include "Utils.h"

CycleDetector::CycleDetector() {
    // void
}

CycleDetector::~CycleDetector() {
    if (_opsHistory != nullptr) {
        delete[] _opsHistory;
        delete[] _findPeriodBuf;
    }
}

void CycleDetector::setHangSamplePeriod(int period) {
    if (_opsHistory != nullptr) {
        delete[] _opsHistory;
        delete[] _findPeriodBuf;
    }
    // May access up to three instructions per execution step
    int size = (period + 2) * 3;
    // May sample up to three periods (as previous hang check may take that long)
    size *= 3;
    _opsHistory = new CycleInstruction[size];
    _findPeriodBuf = new int[size];
    _opsHistoryP = _opsHistory;
    _opsHistoryMaxP = _opsHistoryP + size; // Exclusive
}

void CycleDetector::clearInstructionHistory() {
    _opsHistoryP = _opsHistory;
}

int CycleDetector::getCyclePeriod() {
    return findPeriod(_opsHistory, _findPeriodBuf, (int)(_opsHistoryP - _opsHistory));
}

void CycleDetector::dump() {
    CycleInstruction* opsP = _opsHistory;
    while (opsP < _opsHistoryP) {
        std::cout << (int)(*(opsP++)) << " ";
    }
    std::cout << std::endl;
}
