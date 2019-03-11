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

ProgressTracker::ProgressTracker(ExhaustiveSearcher& searcher) :
    _searcher(searcher),
    _bestProgram(searcher.getProgram().getWidth(), searcher.getProgram().getHeight())
{
    _startTime = clock();

    for (int i = 0; i < numHangTypes; i++) {
        _totalHangsByType[i] = 0;
        _totalErrorsByType[i] = 0;
    }
}

void ProgressTracker::reportDone(int totalSteps) {
    _totalSuccess++;

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "Faulty hang, type = " << (int)_detectedHang
        << ", steps = " << totalSteps << std::endl;
        _searcher.getProgram().dump();
        _searcher.dumpInstructionStack();

        _detectedHang = HangType::UNDETECTED;
    }

    if (totalSteps > _maxStepsSofar) {
        _maxStepsSofar = totalSteps;
        _searcher.getProgram().clone(_bestProgram);
        if (_maxStepsSofar > _dumpBestSofarLimit) {
            std::cout << "Best sofar = " << _maxStepsSofar << std::endl;
            _bestProgram.dump();
            _searcher.dumpInstructionStack();
            _searcher.getData().dump();
            dumpStats();
        }
    }

#ifdef TRACK_EQUIVALENCE
    _equivalenceTotal += _searcher.getProgram().getEquivalenceNumber();
#endif
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
    if (_total % _dumpStackPeriod == 0) {
        _searcher.dumpInstructionStack();
    }
}

void ProgressTracker::reportError() {
    if (_detectedHang != HangType::UNDETECTED) {
        // Hang correctly signalled
        _totalErrorsByType[(int)_detectedHang]++;

        _detectedHang = HangType::UNDETECTED;
    } else {
        _totalErrorsByType[(int)HangType::UNDETECTED]++;
    }

#ifdef TRACK_EQUIVALENCE
    _equivalenceTotal += _searcher.getProgram().getEquivalenceNumber();
#endif
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportAssumedHang() {
    if (_detectedHang != HangType::UNDETECTED) {
        // Hang correctly signalled
        _totalHangsByType[(int)_detectedHang]++;

        _detectedHang = HangType::UNDETECTED;
    } else {
        _totalHangsByType[(int)HangType::UNDETECTED]++;

        if (_dumpUndetectedHangs) {
            std::cout << "Undetected hang" << std::endl;
            _searcher.getProgram().dump();
            _searcher.dumpInstructionStack();
            _searcher.getData().dump();
            std::cout << std::endl;
        }
    }

#ifdef TRACK_EQUIVALENCE
    _equivalenceTotal += _searcher.getProgram().getEquivalenceNumber();
#endif
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportDetectedHang(HangType hangType) {
    if (_searcher.getHangDetectionTestMode()) {
        // Only signal it. The run continues so it can be verified if it was correctly signalled.
        _detectedHang = hangType;
        return;
    }

//    std::cout << "Detected hang" << std::endl;
//    _searcher.getProgram().dump();
//    _searcher.dumpInstructionStack();
//    _searcher.getData().dump();

    _totalHangsByType[(int)hangType]++;

#ifdef TRACK_EQUIVALENCE
    _equivalenceTotal += _searcher.getProgram().getEquivalenceNumber();
#endif
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

long ProgressTracker::getTotalDetectedErrors() {
    long total = 0;
    for (int i = 0; i < numDetectedHangTypes; i++) {
        total += _totalErrorsByType[i];
    }
    return total;
}

long ProgressTracker::getTotalDetectedHangs() {
    long total = 0;
    for (int i = 0; i < numDetectedHangTypes; i++) {
        total += _totalHangsByType[i];
    }
    return total;
}

long ProgressTracker::getTotalErrors() {
    return getTotalDetectedErrors() + _totalErrorsByType[(int)HangType::UNDETECTED];
}

long ProgressTracker::getTotalHangs() {
    return getTotalDetectedHangs() + _totalHangsByType[(int)HangType::UNDETECTED];
}


void ProgressTracker::dumpStats() {
    std::cout
    << "Best=" << _maxStepsSofar
    << ", Total=" << _total
#ifdef TRACK_EQUIVALENCE
    << "/" << _equivalenceTotal
#endif
    << ", Success=";
    if (_searcher.getHangDetectionTestMode()) {
         std::cout << _totalFaultyHangs << "/";
    }
    std::cout << _totalSuccess
    << ", Errors=";
    if (_searcher.getHangDetectionTestMode()) {
        std::cout << getTotalDetectedErrors() << "/";
    }
    std::cout << getTotalErrors()
    << ", Hangs=" << getTotalDetectedHangs() << "/" << getTotalHangs()
    << ", Time taken=" << (clock() - _startTime) / (double)CLOCKS_PER_SEC
    << std::endl;
}

void ProgressTracker::dumpHangStats() {
    std::cout
    << "Hang details:" << std::endl
    << "  #Periodic = " << _totalHangsByType[(int)HangType::PERIODIC] << std::endl
    << "  #Regular Sweep = " << _totalHangsByType[(int)HangType::REGULAR_SWEEP] << std::endl
    << "  #Undetected = " << _totalHangsByType[(int)HangType::UNDETECTED] << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    dumpHangStats();
    _bestProgram.dump();
}
