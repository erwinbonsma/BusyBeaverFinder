//
//  ProgressTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
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

    if (_earlyHangSignalled) {
        // Hang incorrectly signalled
        _earlyHangSignalled = false;
        _totalFaultyHangs++;

        std::cout << "Faulty hang, steps = " << totalSteps << std::endl;
        _searcher->getProgram().dump();
        _searcher->dumpOpStack();
    }

    if (totalSteps > _maxStepsSofar) {
        _maxStepsSofar = totalSteps;
        _searcher->getProgram().clone(_bestProgram);
        if (_maxStepsSofar > 256) {
            std::cout << "Best sofar = " << _maxStepsSofar << std::endl;
            _bestProgram.dump();
            _searcher->dumpOpStack();
            _searcher->getData().dump();
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

    if (_earlyHangSignalled) {
        // Hang correctly signalled
        _earlyHangSignalled = false;
        _totalEarlyErrors++;
    }

    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportHang() {
    _totalHangs++;

    if (_earlyHangSignalled) {
        // Hang correctly signalled
        _earlyHangSignalled = false;
        _totalEarlyHangs++;
    }

    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportEarlyHang() {
    if (_searcher->getHangDetectionTestMode()) {
        // Only signal it. The run continues so it can be verified if it was correctly signalled.
        _searcher->getProgram().dump();
        _searcher->getData().dump();
        _searcher->dumpOpStack();
        _earlyHangSignalled = true;
    } else {
        _totalHangs++;
        _totalEarlyHangs++;
    }
}

void ProgressTracker::dumpStats() {
    std::cout
    << "Best=" << _maxStepsSofar
    << ", Total=" << _total
    << ", Success=" << _totalFaultyHangs << "/" << _totalSuccess
    << ", Errors=" << _totalEarlyErrors << "/" << _totalError
    << ", Hangs=" << _totalEarlyHangs << "/" << _totalHangs
    << ", Time taken=" << (clock() - _startTime) / (double)CLOCKS_PER_SEC
    << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    _bestProgram.dump();
}
