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

void ProgressTracker::report() {
    if (++_total % _dumpStatsPeriod == 0) {
        dumpStats();
    }
    if (_total % _dumpStackPeriod == 0) {
        _searcher.dumpInstructionStack();
    }

//    _searcher.dumpInstructionStack();
//    _searcher.getProgram().dump();
//    _searcher.getInterpretedProgram().dump();
//    _searcher.getProgram().dumpWeb();
//    _searcher.dumpExecutionState();

//    if (_searcher.atTargetProgram()) {
//        std::cout << "Target program generated" << std::endl;
//        _searcher.getProgram().dump();
//        _searcher.dumpInstructionStack();
//    }
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

    report();
}

void ProgressTracker::reportError() {
    if (_detectedHang != HangType::UNDETECTED) {
        // Hang correctly signalled
        _totalErrorsByType[(int)_detectedHang]++;

        _detectedHang = HangType::UNDETECTED;
    } else {
        _totalErrorsByType[(int)HangType::UNDETECTED]++;
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

//        _searcher.dumpInstructionStack();
//        _searcher.getProgram().dump();
//        _searcher.getInterpretedProgram().dump();
//        _searcher.getProgram().dumpWeb();

        if (_dumpUndetectedHangs) {
            std::cout << "Undetected hang: ";
            _searcher.dumpInstructionStack();
        }
    }

    report();
}

void ProgressTracker::reportLateEscape(int numSteps) {
    _totalLateEscapes++;

    std::cout << "Late escape (" << numSteps << "): ";
    _searcher.dumpInstructionStack();

    if (_detectedHang != HangType::UNDETECTED) {
        // Hang incorrectly signalled
        _totalFaultyHangs++;

        std::cout << "Faulty hang, type = " << (int)_detectedHang;
        _searcher.getProgram().dump();
        _searcher.dumpInstructionStack();

        _detectedHang = HangType::UNDETECTED;
    }

    report();
}

void ProgressTracker::reportDetectedHang(HangType hangType) {
    if (_searcher.getNumSteps() > _maxStepsUntilHangDetection) {
        _maxStepsUntilHangDetection = _searcher.getNumSteps();
//        std::cout << "New step limit: ";
//        _searcher.dumpInstructionStack();
//        _searcher.getProgram().dumpWeb();
    }

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

    report();
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
    << ", Fast execs=" << getTotalLateEscapes() << "/" << getTotalFastExecutions()
    << ", Time taken=" << (clock() - _startTime) / (double)CLOCKS_PER_SEC
    << ", Max steps until hang detected=" << _maxStepsUntilHangDetection << " steps"
    << std::endl;
}

void ProgressTracker::dumpHangStats() {
    std::cout
    << "Hang details:" << std::endl
    << "  #No Data Loop = " << _totalHangsByType[(int)HangType::NO_DATA_LOOP] << std::endl
    << "  #No Exit = " << _totalHangsByType[(int)HangType::NO_EXIT] << std::endl
    << "  #Periodic = " << _totalHangsByType[(int)HangType::PERIODIC] << std::endl
    << "  #Regular Sweep = " << _totalHangsByType[(int)HangType::REGULAR_SWEEP] << std::endl
    << "  #Glider = " << _totalHangsByType[(int)HangType::APERIODIC_GLIDER] << std::endl
    << "  #Undetected = " << _totalHangsByType[(int)HangType::UNDETECTED] << std::endl;
}

void ProgressTracker::dumpFinalStats() {
    dumpStats();
    dumpHangStats();
    _bestProgram.dump();
}
