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
#include "HangDetector.h"
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

    _lastDetectedHang = nullptr;
}

void ProgressTracker::report() {
//    if (_searcher.getProgram().toString().rfind("Zv7+ktS9FoAlaw", 0) == 0) {
//        std::cout << "At target program" << std::endl;
//        _searcher.getProgram().dump();
//        _searcher.dumpInstructionStack();
//    }

    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
        _searcher.dumpInstructionStack();
    }

//    _searcher.getProgram().dump();
//    _searcher.getInterpretedProgram().dump();
//    _searcher.dumpExecutionState();
//    if (_lastDetectedHang && (
//            _lastDetectedHang->hangType()==HangType::REGULAR_SWEEP ||
//            _lastDetectedHang->hangType()==HangType::IRREGULAR_SWEEP
//    )) {
//        ((const SweepHangDetector *)_lastDetectedHang)->dump();
//    }
//    _searcher.getProgram().dumpWeb();

//    if (_searcher.atTargetProgram()) {
//        std::cout << "Target program generated" << std::endl;
//        _searcher.getProgram().dump();
//        _searcher.dumpInstructionStack();
//    }
}

void ProgressTracker::reportDone(int totalSteps) {
    _totalSuccess++;

    if (_dumpDone) {
        std::cout << "Done(" << totalSteps << "): "
        << _searcher.getProgram().toString() << std::endl;
    }

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "False positive, type = " << (int)_detectedHang << ", steps = " << totalSteps
        << ": " << _searcher.getProgram().toString() << std::endl;

        _detectedHang = HangType::UNDETECTED;
    }

    if (totalSteps > _maxStepsSofar) {
        _maxStepsSofar = totalSteps;
        _searcher.getProgram().clone(_bestProgram);
        if (_maxStepsSofar > _dumpBestSofarLimit) {
            std::cout << "Best sofar = " << _maxStepsSofar
            << ": " << _bestProgram.toString() << std::endl;
//            _searcher.getData().dump();
            dumpStats();
        }
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
            std::cout << "Error: " << _searcher.getProgram().toString() << std::endl;
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

//        _searcher.getProgram().dump();
//        _searcher.getInterpretedProgram().dump();
//        _searcher.getProgram().dumpWeb();

        if (_dumpUndetectedHangs) {
            std::cout << "Undetected hang: " << _searcher.getProgram().toString() << std::endl;
//            _searcher.getProgram().dump();
        }
    }

    report();
}

void ProgressTracker::reportLateEscape(int numSteps) {
    _totalLateEscapes++;

    std::cout << "Late escape (" << numSteps << "): "
    << _searcher.getProgram().toString() << std::endl;

    std::cout << "ESC " << numSteps << " ";
    _searcher.dumpInstructionStack(" ");

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "False positive, type = " << (int)_detectedHang
        << ": " << _searcher.getProgram().toString() << std::endl;

        _detectedHang = HangType::UNDETECTED;
    }

    report();
}

void ProgressTracker::reportDetectedHang(HangType hangType, bool executionWillContinue) {
    _maxStepsUntilHangDetection = std::max(_maxStepsUntilHangDetection,
                                           _searcher.getProgramExecutor()->numSteps());
    if (executionWillContinue) {
        // Only signal it. Execution will continue to verify that the program indeed hangs.
        _detectedHang = hangType;
        return;
    }

//    std::cout << "Detected hang: " << _searcher.getProgram().toString() << std::endl;
//    _searcher.getProgram().dump();
//    _searcher.getData().dump();

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
    double time = (clock() - _startTime) / (double)CLOCKS_PER_SEC;

    std::cout << time
    << ": Max steps=" << _maxStepsSofar
    << ", Max hang detection steps=" << _maxStepsUntilHangDetection
    << std::endl;

    std::cout << time
    << ": Program=" << _searcher.getProgram().toString()
    << ", Stack=";
    _searcher.dumpInstructionStack(",");

    std::cout << time
    << ": Total=" << _total
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
    << ", Hangs=" << (getTotalHangs() - getTotalDetectedHangs()) << "/" << getTotalDetectedHangs()
    << ", Fast execs=" << getTotalLateEscapes() << "/" << getTotalFastExecutions()
    << std::endl;

    std::cout << time
    << ": NODATA=" << _totalHangsByType[(int)HangType::NO_DATA_LOOP]
    << ", NOEXIT=" << _totalHangsByType[(int)HangType::NO_EXIT]
    << ", PERIOD=" << _totalHangsByType[(int)HangType::PERIODIC]
    << ", METAPE=" << _totalHangsByType[(int)HangType::META_PERIODIC]
    << ", RSWEEP=" << _totalHangsByType[(int)HangType::REGULAR_SWEEP]
    << "  ISWEEP=" << _totalHangsByType[(int)HangType::IRREGULAR_SWEEP]
    << "  GLIDER=" << _totalHangsByType[(int)HangType::APERIODIC_GLIDER]
    << "  ASUMED=" << _totalHangsByType[(int)HangType::UNDETECTED]
    << std::endl;
}

void ProgressTracker::dumpHangStats() {
    std::cout
    << "Hang details:" << std::endl
    << "  #No Data Loop = " << _totalHangsByType[(int)HangType::NO_DATA_LOOP] << std::endl
    << "  #No Exit = " << _totalHangsByType[(int)HangType::NO_EXIT] << std::endl
    << "  #Periodic = " << _totalHangsByType[(int)HangType::PERIODIC] << std::endl
    << "  #Meta-periodic = " << _totalHangsByType[(int)HangType::META_PERIODIC] << std::endl
    << "  #Regular Sweep = " << _totalHangsByType[(int)HangType::REGULAR_SWEEP] << std::endl
    << "  #Irregular Sweep = " << _totalHangsByType[(int)HangType::IRREGULAR_SWEEP] << std::endl
    << "  #Glider = " << _totalHangsByType[(int)HangType::APERIODIC_GLIDER] << std::endl
    << "  #Undetected = " << _totalHangsByType[(int)HangType::UNDETECTED] << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    dumpHangStats();
    _bestProgram.dump();
}
