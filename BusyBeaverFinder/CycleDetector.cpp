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

void CycleDetector::setCapacity(int capacity) {
    if (_opsHistory != nullptr) {
        delete[] _opsHistory;
        delete[] _findPeriodBuf;
    }
    _opsHistory = new CycleInstruction[capacity];
    _findPeriodBuf = new int[capacity];
    _opsHistoryP = _opsHistory;
    _opsHistoryMaxP = _opsHistoryP + capacity; // Exclusive
}

int CycleDetector::getCyclePeriod() {
    return findPeriod(_opsHistory, _findPeriodBuf, (int)(_opsHistoryP - _opsHistory));
}

int CycleDetector::getCyclePeriod(int fromSampleIndex) {
    CycleInstruction* sampleStartP = _opsHistory + fromSampleIndex;
    return findPeriod(sampleStartP, _findPeriodBuf, (int)(_opsHistoryP - sampleStartP));
}

void CycleDetector::dump() {
    CycleInstruction* opsP = _opsHistory;
    while (opsP < _opsHistoryP) {
        std::cout << (int)(*(opsP++)) << " ";
    }
    std::cout << std::endl;
}
