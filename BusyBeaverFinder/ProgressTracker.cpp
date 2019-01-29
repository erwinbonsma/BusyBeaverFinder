//
//  ProgressTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include "ProgressTracker.h"

#include <iostream>

#include "ExhaustiveSearcher.h"
#include "Program.h"

ProgressTracker::ProgressTracker(ExhaustiveSearcher *searcher) :
    _bestProgram(searcher->getProgram().getWidth(), searcher->getProgram().getHeight())
{
    _searcher = searcher;
    _startTime = clock();
}

void ProgressTracker::reportDone(int totalSteps) {
    _totalSuccess++;
    if (totalSteps > _maxStepsSofar) {
        _maxStepsSofar = totalSteps;
        _searcher->getProgram().clone(_bestProgram);
        if (_maxStepsSofar > 256) {
            std::cout << "Best sofar = " << _maxStepsSofar << std::endl;
            _bestProgram.dump();
            _searcher->getData().dump();
            _searcher->dumpOpStack();
        }
    }
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
    if (_total % _dumpStackPeriod == 0) {
        _searcher->dumpOpStack();
    }
}

void ProgressTracker::reportError() {
    _totalError++;
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportHang(bool early) {
    _totalHangs++;
    if (early) {
        _totalEarlyHangs++;
    }
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::dumpStats() {
    std::cout
    << "Best=" << _maxStepsSofar
    << ", Total=" << _total
    << ", Success=" << _totalSuccess
    << ", Errors=" << _totalError
    << ", Hangs=" << _totalEarlyHangs << "/" << _totalHangs
    << ", Time taken=" << (clock() - _startTime) / (double)CLOCKS_PER_SEC
    << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    _bestProgram.dump();
}
