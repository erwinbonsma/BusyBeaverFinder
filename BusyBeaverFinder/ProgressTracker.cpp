//
//  ProgressTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ProgressTracker.h"

#include <iostream>

#include "Searcher.h"
#include "HangDetector.h"
#include "Program.h"

ProgressTracker::ProgressTracker() :
    _runLengthHistogram(),
    _hangDetectionHistogram(3, 2)
{
    _startTime = clock();

    for (int i = 0; i < numHangTypes; i++) {
        _totalHangsByType[i] = 0;
        _totalErrorsByType[i] = 0;
    }

    _lastDetectedHang = nullptr;
}

void ProgressTracker::report() {
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
}

void ProgressTracker::reportDone(int totalSteps) {
    _totalSuccess++;
    _runLengthHistogram.add(totalSteps);

    if (totalSteps > _dumpSuccessStepsLimit) {
        std::cout << "SUC " << totalSteps
        << " " << _searcher->getProgram().toString() << std::endl;
    }

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "False positive, type = " << (int)_detectedHang << ", steps = " << totalSteps
        << ": " << _searcher->getProgram().toString() << std::endl;

        _detectedHang = HangType::UNDETECTED;
    }

    if (totalSteps > _maxStepsSofar) {
        _maxStepsSofar = totalSteps;
        _searcher->getProgram().clone(_bestProgram);
    }

    report();
}

void ProgressTracker::reportError() {
    if (_detectedHang != HangType::UNDETECTED) {
        // Hang correctly signalled
        _totalErrorsByType[(int)_detectedHang]++;

        _detectedHang = HangType::UNDETECTED;
    } else {
        _totalErrorsByType[(int)HangType::UNDETECTED]++;

        if (_dumpUndetectedHangs) {
            std::cout << "ERR " << _searcher->getProgram().toString() << std::endl;
        }
    }

    report();
}

void ProgressTracker::reportAssumedHang() {
    if (_detectedHang != HangType::UNDETECTED) {
        // Hang correctly signalled
        _totalHangsByType[(int)_detectedHang]++;

        _detectedHang = HangType::UNDETECTED;
    } else {
        _totalHangsByType[(int)HangType::UNDETECTED]++;

        if (_dumpUndetectedHangs) {
            // Dump the assumed hang
            std::cout << "ASS " << _searcher->getProgram().toString() << std::endl;
        }
    }

    report();
}

void ProgressTracker::reportLateEscape(int numSteps) {
    _totalLateEscapes++;

    std::cout << "ESC " << numSteps << " "
    << _searcher->getProgram().toString() << std::endl;

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "False positive, type = " << (int)_detectedHang
        << ": " << _searcher->getProgram().toString() << std::endl;

        _detectedHang = HangType::UNDETECTED;
    }

    // Do not count late escapes as programs.
    // They will expand into multiple programs when resuming the search from these late escapes.
}

void ProgressTracker::reportDetectedHang(HangType hangType, bool executionWillContinue) {
    int numSteps = _searcher->getNumSteps();
    _maxStepsUntilHangDetection = std::max(_maxStepsUntilHangDetection, numSteps);
    _hangDetectionHistogram.add(numSteps);

    if (executionWillContinue) {
        // Only signal it. Execution will continue to verify that the program indeed hangs.
        _detectedHang = hangType;
        return;
    }

    _totalHangsByType[(int)hangType]++;

    report();
}

void ProgressTracker::reportDetectedHang(std::shared_ptr<HangDetector> hangDetector,
                                         bool executionWillContinue) {
    _lastDetectedHang = hangDetector;

    reportDetectedHang(hangDetector->hangType(), executionWillContinue);
}

long ProgressTracker::getTotalDetectedErrors() const {
    long total = 0;
    for (int i = 0; i < numDetectedHangTypes; i++) {
        total += _totalErrorsByType[i];
    }
    return total;
}

long ProgressTracker::getTotalDetectedHangs() const {
    long total = 0;
    for (int i = 0; i < numDetectedHangTypes; i++) {
        total += _totalHangsByType[i];
    }
    return total;
}

long ProgressTracker::getTotalErrors() const {
    return getTotalDetectedErrors() + _totalErrorsByType[(int)HangType::UNDETECTED];
}

long ProgressTracker::getTotalHangs() const {
    return getTotalDetectedHangs() + _totalHangsByType[(int)HangType::UNDETECTED];
}


void ProgressTracker::dumpStats() {
    _timeStamp = (clock() - _startTime) / (double)CLOCKS_PER_SEC;

    std::cout << _timeStamp
    << ": Total=" << _total
    << ", Success=";
//    if (_searcher.getHangDetectionTestMode()) {
//        std::cout << _totalFaultyHangs << "/";
//    }
    std::cout << _totalSuccess
    << ", Errors=";
//    if (_searcher.getHangDetectionTestMode()) {
//        std::cout << getTotalDetectedErrors() << "/";
//    }
    std::cout << getTotalErrors()
    << ", Hangs=" << (getTotalHangs() - getTotalDetectedHangs()) << "/" << getTotalDetectedHangs()
    << ", Fast execs=" << getTotalLateEscapes() << "/" << getTotalFastExecutions()
    << std::endl;

    if (_searcher) {
        std::cout << _timeStamp << ": ";
        _searcher->dumpSearchProgress(std::cout);
        std::cout << std::endl;
    }

    std::cout << _timeStamp
    << ": Max steps=" << _maxStepsSofar
    << ", Program=" << _bestProgram.toString()
    << std::endl;

    dumpRunLengths();
    dumpHangStats();
}

void ProgressTracker::dumpRunLengths() {
    std::cout << _timeStamp << ": Run lengths = " << _runLengthHistogram << std::endl;
}

void ProgressTracker::dumpHangStats() {
    std::cout << _timeStamp
    << ": Hang detected at = " << _hangDetectionHistogram
    << ", max steps = " << _maxStepsUntilHangDetection
    << std::endl;

    std::cout << _timeStamp
    << ": NODATA=" << _totalHangsByType[(int)HangType::NO_DATA_LOOP]
    << ", NOEXIT=" << _totalHangsByType[(int)HangType::NO_EXIT]
    << ", PERIOD=" << _totalHangsByType[(int)HangType::PERIODIC]
    << ", NESTED=" << _totalHangsByType[(int)HangType::NESTED_PERIODIC]
    << ", RSWEEP=" << _totalHangsByType[(int)HangType::REGULAR_SWEEP]
    << ", ISWEEP=" << _totalHangsByType[(int)HangType::IRREGULAR_SWEEP]
    << ", GLIDER=" << _totalHangsByType[(int)HangType::APERIODIC_GLIDER]
    << ", ASUMED=" << _totalHangsByType[(int)HangType::UNDETECTED]
    << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    _bestProgram.dump();
}
