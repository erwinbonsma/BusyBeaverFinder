//
//  IrregularSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "IrregularSweepHangDetector.h"

int numIrregularSweepFailures = 0;
bool irregularSweepHangFailure(const ExecutionState& execution) {
//    execution.dumpExecutionState();
    numIrregularSweepFailures++;
    return false;
}

bool irregularSweepHangFailure(const IrregularSweepTransitionGroup& transitionGroup) {
    numIrregularSweepFailures++;
    return false;
}

std::set<int> tmpExhibitsAperiodicGrowthSet;
bool IrregularSweepTransitionGroup::exhibitsAperiodicGrowth() {
    auto &exitDeltas = tmpExhibitsAperiodicGrowthSet;
    exitDeltas.clear();

    bool hasNegativeDeltas = false;
    for (int exitDelta : _sweepExitDeltas) {
        exitDeltas.insert(exitDelta);
        if (exitDelta < 0) {
            hasNegativeDeltas = true;
        }
    }

    return (hasNegativeDeltas && exitDeltas.size() >= 3);
}

bool IrregularSweepTransitionGroup::determineZeroExitSweepEndType() {
    if (SweepTransitionGroup::determineZeroExitSweepEndType()) {
        return true;
    }

    if (exhibitsAperiodicGrowth()) {
        setEndType(SweepEndType::FIXED_APERIODIC_APPENDIX);
    }

    return didDetermineEndType();
}

bool IrregularSweepTransitionGroup::determineSweepEndType() {
    if (!SweepTransitionGroup::determineSweepEndType()) {
        return false;
    }

    if (endType() == SweepEndType::FIXED_APERIODIC_APPENDIX) {
        bool exitsOnZero = false;
        int exitCount = 0;

        auto loop = incomingLoop();
        if (abs(loop->dataPointerDelta()) != 1) {
            // The loop over the appendix should not be able to skip over zeroes by moving DP more
            // than one unit each iteration. This is needed to avoid false positives for some long-
            // running yet finite 7x7 programs
            return irregularSweepHangFailure(*this);
        }

        for (int i = loop->loopSize(); --i >= 0; ) {
            auto loopExit = loop->exit(i);

            if (loopExit.exitWindow == ExitWindow::ANYTIME) {
                exitCount += 1;
                if (loopExit.exitCondition.isTrueForValue(0)) {
                    exitsOnZero = true;
                }
            }
        }

        // The incoming loop should have exactly two anytime exits, with one exiting at zero (which
        // extends the appendix)
        if (exitCount != 2 || !exitsOnZero) {
            return irregularSweepHangFailure(*this);
        }
    }

    return true;
}

IrregularSweepHangDetector::IrregularSweepHangDetector(const ExecutionState& execution)
: SweepHangDetector(execution) {
    for (int i = 0; i < 2; i++ ) {
        _transitionGroups[i] = new IrregularSweepTransitionGroup();
    }
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
            return irregularSweepHangFailure(_execution);
        }

        addSweepLength(transitionScanner.lastSweepLength());

        if (st->numOccurences == 1) {
            ++numUniqueTransitions;
        } else if (st->numOccurences == 2) {
            --numUniqueTransitions;
        }
    }

    populateExitDeltas();

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
        return irregularSweepHangFailure(_execution);
    }

    return true;
}
