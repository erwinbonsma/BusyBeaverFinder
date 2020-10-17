//
//  StaticSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticSweepHangDetector.h"

#include <iostream>

int numFailed = 0;
bool failed(ExhaustiveSearcher& searcher) {
    numFailed++;
    searcher.dumpHangDetection();
    return false;
}

bool SweepLoopAnalysis::analyseSweepLoop(RunBlock* runBlock, ExhaustiveSearcher& searcher) {
    if (!analyseLoop(searcher.getInterpretedProgram(),
                     searcher.getRunSummary(),
                     runBlock->getStartIndex(),
                     runBlock->getLoopPeriod())) {
        return failed(searcher);
    }

    if (abs(dataPointerDelta()) != 1) {
        // TODO: Support loops that move more than one cell per iteration
        return failed(searcher);
    }

    if (numBootstrapCycles() > 0) {
        // TODO: Support loops with bootstrap
        return failed(searcher);
    }

    int numAnytimeExits = 0;
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            numAnytimeExits++;

            if (anyDataDeltasUpUntil(i)) {
                // TODO: Support loops that through an early exit make a change
                return failed(searcher);
            }

            if (!exit(i).exitCondition.isTrueForValue(0)) {
                // TODO: Support loops that exit on non-zero
                return failed(searcher);
            }
        }
    }

    if (numAnytimeExits > 1) {
        // TODO: Support more than one exit
        return failed(searcher);
    }

    if (numDataDeltas() > 0) {
        // TODO: Support loops that not only move, but also modify
        return failed(searcher);
    }

    dump();
    return true;
}

bool SweepTransitionAnalysis::analyseSweepTransition(RunBlock* runBlock, bool atRight,
                                                     ExhaustiveSearcher& searcher) {
    RunSummary& runSummary = searcher.getRunSummary();
    InterpretedProgram& interpretedProgram = searcher.getInterpretedProgram();

    // The instructions comprising the (last) transition sequence
    int startIndex = runBlock->getStartIndex();
    int len = (runBlock + 1)->getStartIndex() - startIndex;

    if (!analyseSequence(interpretedProgram, runSummary, startIndex, len)) {
        return failed(searcher);
    }

    _extendsSweep = false; // Default
    for (int i = numDataDeltas(); --i >= 0; ) {
        DataDelta &dd = dataDeltaAt(i);

        if (dd.dpOffset() == 0) {
            _extendsSweep = true;
        }
    }

    dump();
    return true;
}

void SweepTransitionAnalysis::dump() {
    SequenceAnalysis::dump();

    std::cout << "Extends = " << _extendsSweep << std::endl;
}

StaticSweepHangDetector::StaticSweepHangDetector(ExhaustiveSearcher& searcher)
    : StaticHangDetector(searcher) {}

bool StaticSweepHangDetector::analyseLoops() {
    // Assume that the loop which just finished is one of the sweep loops
    RunSummary& runSummary = _searcher.getRunSummary();

    _loopRunBlock[1] = runSummary.getLastRunBlock();
    if (!_loop[1].analyseSweepLoop(_loopRunBlock[1], _searcher)) {
        return failed(_searcher);
    }

    _loopRunBlock[0] = _loopRunBlock[1] - 2;
    if (!_loop[0].analyseSweepLoop(_loopRunBlock[0], _searcher)) {
        return failed(_searcher);
    }

    // Both loops should move in opposite directions
    if (_loop[0].dataPointerDelta() * _loop[1].dataPointerDelta() >= 0) {
        return failed(_searcher);
    }

    return true;
}

bool StaticSweepHangDetector::analyseTransitions() {
    _transitionRunBlock[0] = _loopRunBlock[0] - 1;
    if (!_transition[0].analyseSweepTransition(_transitionRunBlock[0],
                                               _loop[0].dataPointerDelta() < 0,
                                               _searcher)) {
        return failed(_searcher);
    }

    _transitionRunBlock[1] =_loopRunBlock[1] - 1;
    if (!_transition[1].analyseSweepTransition(_transitionRunBlock[1],
                                               _loop[1].dataPointerDelta() < 0,
                                               _searcher)) {
        return failed(_searcher);
    }

    // At least one side should grow
    if (!(_transition[0].extendsSweep() || _transition[1].extendsSweep())) {
        return failed(_searcher);
    }

    return true;
}

bool StaticSweepHangDetector::onlyZeroesAhead(bool atRight, bool skipNonZeroes) {
    Data& data = _searcher.getData();
    int delta = atRight ? 1 : -1;
    DataPointer dpStart = data.getDataPointer();
    DataPointer dpEnd = atRight ? data.getMaxDataP() : data.getMinDataP();
    DataPointer dp = dpStart;

    // DP is at the other side of the sweep. Find the other end of the sweep.
    if (skipNonZeroes) {
        dp += delta;
        while (*dp) {
            if (!*dp) {
                // Found end of sweep at other end
                break;
            }

            assert(dp != dpEnd);

            dp += delta;
        }
    }

    assert(!*dp);
    while (true) {
        if (*dp) {
            return failed(_searcher);
        }

        if (dp == dpEnd) {
            break;
        }

        dp += delta;
    }

    return true;
}

bool StaticSweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return !loopContinues && _searcher.getMetaRunSummary().isInsideLoop();
}

bool StaticSweepHangDetector::analyzeHangBehaviour() {
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    RunBlock* metaRunBlock = metaRunSummary.getLastRunBlock();
    int metaPeriod = metaRunBlock->getLoopPeriod();

    if (metaPeriod != 4) {
        // For now, assume that each transition sequence consists of a single run block (i.e. it
        // does not contain a fixed loop)
        return failed(_searcher);
    }

    if (!analyseLoops()) {
        return failed(_searcher);
    }

    if (!analyseTransitions()) {
        return failed(_searcher);
    }

    return true;
}

Trilian StaticSweepHangDetector::proofHang() {
    _searcher.getData().dump();
    if (_transition[0].extendsSweep() &&
        !onlyZeroesAhead(_loop[0].dataPointerDelta() > 0, true)) {
        return Trilian::MAYBE;
    }

    if (_transition[1].extendsSweep() &&
        !onlyZeroesAhead(_loop[1].dataPointerDelta() > 0, false)) {
        return Trilian::MAYBE;
    }

    return Trilian::YES;
}
