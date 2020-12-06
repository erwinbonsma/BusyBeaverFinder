//
//  IrregularSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "IrregularSweepHangDetector.h"

int numIrregularSweepFailures = 0;
bool irregularSweepHangFailure(const ProgramExecutor& executor) {
//    executor.dumpExecutionState();
    numIrregularSweepFailures++;
    return false;
}

IrregularSweepHangDetector::IrregularSweepHangDetector(const ProgramExecutor& executor)
: SweepHangDetector(executor) {
    for (int i = 0; i < 2; i++ ) {
        _transitionGroups[i] = new SweepTransitionGroup();
    }
}

bool IrregularSweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return !loopContinues;
}

bool IrregularSweepHangDetector::analyzeTransitions() {
    SweepTransitionScanner transitionScanner(*this);
    int numUniqueTransitions = 0;

    // Each transition should occur at least twice. This guards against bootstrap-only effects.
    // Furthermore, check at least 8 sweeps. The incoming loop for the assumed irregular appendix
    // should at least have two exits, and we want to have at least two instances of the
    // transitions that follow it.
    while (numUniqueTransitions != 0 || transitionScanner.numSweeps() < 8) {
        const SweepTransition* st = transitionScanner.analyzePreviousSweepTransition();
        if (st == nullptr) {
//            std::cout << std::endl;
            return irregularSweepHangFailure(_executor);
        }

//        std::cout << transitionScanner.numSweeps() << ":" << numUniqueTransitions << " ";
        if (st->numOccurences == 1) {
            ++numUniqueTransitions;
        } else if (st->numOccurences == 2) {
            --numUniqueTransitions;
        }
    }

//    std::cout << "numSweeps = " << transitionScanner.numSweeps() << std::endl;
//    if (transitionScanner.numSweeps() == 8) {
//        return false; // TEMP
//    }

//    if (transitionScanner.numSweeps() > 8) {
//        _executor.getData().dump();
//    }

    return true;
}

bool IrregularSweepHangDetector::analyzeHangBehaviour() {
    if (!SweepHangDetector::analyzeHangBehaviour()) {
        return false;
    }

//    dump();
//    std::cout << _transitionGroups[0]->endType() << " " << _transitionGroups[1]->endType()
//    << std::endl;

    // An irregular sweep should have at least one a-periodic end point
    if (! (_transitionGroups[0]->endType() == SweepEndType::FIXED_APERIODIC_APPENDIX ||
           _transitionGroups[1]->endType() == SweepEndType::FIXED_APERIODIC_APPENDIX)
    ) {
        return irregularSweepHangFailure(_executor);
    }

    return true;
}
