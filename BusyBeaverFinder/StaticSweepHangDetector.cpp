//
//  StaticSweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "StaticSweepHangDetector.h"

#include <iostream>
#include "Utils.h"

int numFailed = 0;
bool failed(ExhaustiveSearcher& searcher) {
    numFailed++;
//    searcher.dumpHangDetection();
//    searcher.getInterpretedProgram().dump();
//    searcher.getRunSummary().dump();
//    searcher.getMetaRunSummary().dump();
    return false;
}


bool SweepLoopAnalysis::isExitValue(int value) {
    return std::find(_exitValues.begin(), _exitValues.end(), value) != _exitValues.end();
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

    _deltaSign = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int sgn = sign(dataDeltaAt(i).delta());

        if (_deltaSign != 0 && _deltaSign != sgn) {
            // TODO?: Support sweep loops that make changes in opposite directions
            return failed(searcher);
        }
        _deltaSign = sgn;
    }

    _exitValues.clear();
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            _exitValues.push_back(exit(i).exitCondition.value());

            if (!exitsOnZero(i)) {
                // TODO?: Support loops that exit on non-zero
                return failed(searcher);
            }
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta) {
    os << (const LoopAnalysis&)sta;
    os << "Exit values: ";
    bool isFirst = true;
    for (int value : sta._exitValues) {
        if (isFirst) {
            isFirst = false;
        } else {
            os << ", ";
        }
        os << value;
    }
    os << std::endl;

    return os;
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
        const DataDelta &dd = dataDeltaAt(i);

        if (dd.dpOffset() == 0) {
            _extendsSweep = true;
        }
    }

    return true;
}

void SweepTransitionAnalysis::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta) {
    os << (const SequenceAnalysis&)sta;
    os << ", extends = " << sta.extendsSweep();

    return os;
}

StaticSweepHangDetector::StaticSweepHangDetector(ExhaustiveSearcher& searcher)
    : StaticHangDetector(searcher) {}

bool StaticSweepHangDetector::analyseLoops() {
    // Assume that the loop which just finished is one of the sweep loops
    RunSummary& runSummary = _searcher.getRunSummary();

    _loopRunBlock[1] = runSummary.getLastRunBlock();
    if (!_loop[1].analyseSweepLoop(_loopRunBlock[1], _searcher)) {
        return false;
    }

    _loopRunBlock[0] = _loopRunBlock[1] - 2;
    if (!_loop[0].analyseSweepLoop(_loopRunBlock[0], _searcher)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (_loop[0].dataPointerDelta() * _loop[1].dataPointerDelta() >= 0) {
        return failed(_searcher);
    }

    if (_loop[0].deltaSign() * _loop[1].deltaSign() == -1) {
        // TODO?: Support loops that makes changes to the sequence in opposite directions
        return failed(_searcher);
    }
    _sweepDeltaSign = _loop[(int)(_loop[0].deltaSign() == 0)].deltaSign();

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

bool StaticSweepHangDetector::scanSweepSequence(
    DataPointer &dp, SweepLoopAnalysis &sweepLoop
) {
    Data& data = _searcher.getData();
    int delta = sweepLoop.dataPointerDelta();
    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();

    // DP is at the other side of the sweep. Find the other end of the sweep.
    dp += delta;
    while (*dp) {
        if (sweepLoop.isExitValue(*dp)) {
            // Found end of sweep at other end
            break;
        }

        if (_sweepDeltaSign * sign(*dp) < 0) {
            // The sweep makes changes to the sequence that move some values towards zero
            data.dump();
            return failed(_searcher);
        }

        assert(dp != dpEnd); // Assumes abs(dataPointerDelta) == 1

        dp += delta;
    }

    return true;
}

bool StaticSweepHangDetector::onlyZeroesAhead(DataPointer &dp, bool atRight) {
    Data& data = _searcher.getData();
    int delta = atRight ? 1 : -1;
    DataPointer dpEnd = atRight ? data.getMaxDataP() : data.getMinDataP();

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

//        _searcher.dumpHangDetection();
//        _searcher.getInterpretedProgram().dump();
//        _searcher.getRunSummary().dump();
//        _searcher.getMetaRunSummary().dump();
//        dump();

        return failed(_searcher);
    }

    if (!analyseLoops()) {
        return false;
    }

    if (!analyseTransitions()) {
        return false;
    }

    dump();

    return true;
}

Trilian StaticSweepHangDetector::proofHang() {
    Data& data = _searcher.getData();
    DataPointer dp1 = data.getDataPointer();
    DataPointer dp0 = dp1; // Initial value

    if (!scanSweepSequence(dp0, _loop[0])) {
        return Trilian::MAYBE;
    }

    if (_transition[0].extendsSweep() &&
        !onlyZeroesAhead(dp0, _loop[0].dataPointerDelta() > 0)) {
        return Trilian::MAYBE;
    }

    if (_transition[1].extendsSweep() &&
        !onlyZeroesAhead(dp1, _loop[1].dataPointerDelta() > 0)) {
        return Trilian::MAYBE;
    }

    return Trilian::YES;
}

void StaticSweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const StaticSweepHangDetector &detector) {
    os << "T#0" << std::endl << detector._transition[0] << std::endl;
    os << "L#0" << std::endl << detector._loop[0];
    os << "T#1" << std::endl << detector._transition[1] << std::endl;
    os << "L#1" << std::endl << detector._loop[1];

    return os;
}
