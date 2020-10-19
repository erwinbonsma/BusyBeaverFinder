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

void SweepTransitionGroup::clear() {
    transitions.clear();
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
    RunSummary& runSummary = _searcher.getRunSummary();
    int i = runSummary.getNumRunBlocks() - 2;
    int numTransitions = 0, numUniqueTransitions = 0;

    _transitionGroup[0].clear();
    _transitionGroup[1].clear();

    while (i > 0) {
        RunBlock* transitionBlock = runSummary.runBlockAt(i);
        RunBlock* loopBlock = runSummary.runBlockAt(i - 1);
        int j = numTransitions % 2;
        SweepTransitionGroup &tg = _transitionGroup[j];

        if (
            !loopBlock->isLoop() ||
            loopBlock->getSequenceIndex() != _loopRunBlock[j]->getSequenceIndex()
        ) {
            // Loops do not follow expected sweep pattern
            break;
        }

        int loopLen = runSummary.getRunBlockLength(i - 1);
        int exitInstruction = loopLen % loopBlock->getLoopPeriod();

        if (tg.transitions.find(exitInstruction) == tg.transitions.end()) {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_UNIQUE_TRANSITIONS_PER_SWEEP);
            SweepTransitionAnalysis *sa = &_transitionPool[numUniqueTransitions++];

            if (!sa->analyseSweepTransition(transitionBlock,
                                            _loop[j].dataPointerDelta() > 0,
                                            _searcher)) {
                return false;
            }

            tg.transitions[exitInstruction] = sa;
        }

        i -= 2;
        numTransitions++;
    }

    if (numTransitions < 4) {
        // The pattern is too short
        return failed(_searcher);
    }

    // TODO: Analyse transition groups
    // 1) Is position fixed, or can it move?
    // 2) Are all possible loop exits covered?

    return true;
}

bool StaticSweepHangDetector::scanSweepSequence(DataPointer &dp, SweepLoopAnalysis &sweepLoop) {
    Data& data = _searcher.getData();
    int delta = sweepLoop.dataPointerDelta();
    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();

    // DP is at the other side of the sweep. Find the other end of the sweep.
    dp += delta;
    while (*dp) {
        if (_loop[0].isExitValue(*dp) || _loop[1].isExitValue(*dp)) {
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
    return !loopContinues;
}

bool StaticSweepHangDetector::analyzeHangBehaviour() {
    RunSummary& runSummary = _searcher.getRunSummary();

    if (runSummary.getNumRunBlocks() <= 8) {
        // The run should contain two full sweeps preceded by a loop: L1 (T1 L0 T0 L1) (T1 L0 T0 L1)
        // Note, transitions are named after the loop that precedes it (as they depend on the exit
        // of that loop).
        return false;
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

    if (
        !_transitionGroup[0].positionIsFixed &&
        !onlyZeroesAhead(dp0, _loop[0].dataPointerDelta() > 0)
    ) {
        return Trilian::MAYBE;
    }

    if (
        !_transitionGroup[1].positionIsFixed &&
        !onlyZeroesAhead(dp1, _loop[1].dataPointerDelta() > 0)
    ) {
        return Trilian::MAYBE;
    }

    return Trilian::YES;
}

void StaticSweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

void dumpTransitions(std::ostream &os, const SweepTransitionGroup &transitionGroup) {
    auto iter = transitionGroup.transitions.begin();
    while (iter != transitionGroup.transitions.end()) {
        os << "  Exit at " << iter->first << ": " << *(iter->second) << std::endl;
        ++iter;
    }
}

std::ostream &operator<<(std::ostream &os, const StaticSweepHangDetector &detector) {
    os << "Loop #0" << std::endl << detector._loop[0];
    dumpTransitions(os, detector._transitionGroup[0]);

    os << "Loop #1" << std::endl << detector._loop[1];
    dumpTransitions(os, detector._transitionGroup[1]);

    return os;
}
