//
//  SweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/10/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#include "SweepHangDetector.h"

#include <iostream>
#include "Utils.h"

const int MAX_ITERATIONS_TRANSITION_LOOPS = 3;
const int MIN_ITERATIONS_LAST_SWEEP_LOOP = 5;

int numFailed = 0;
bool failed(const ProgramExecutor& executor) {
    numFailed++;
    // executor.dumpExecutionState();
    return false;
}

bool failed(SweepTransitionGroup& tg) {
//    std::cout << tg << std::endl;
    numFailed++;
    return false;
}

namespace std {
size_t hash<SweepLoopExit>::operator()(const SweepLoopExit& sle) const {
    return (sle.loop->loopRunBlock()->getSequenceIndex() << 8) ^ sle.exitInstructionIndex;
}
}

bool SweepLoopAnalysis::isExitValue(int value) const {
    return _exitMap.find(value) != _exitMap.end();
}

int SweepLoopAnalysis::numberOfExitsForValue(int value) const {
    return (int)_exitMap.count(value);
}

bool SweepLoopAnalysis::analyzeSweepLoop(const RunBlock* runBlock,
                                         const ProgramExecutor& executor) {
    _loopRunBlock = runBlock;

    if (!analyzeLoop(executor.getInterpretedProgram(),
                     executor.getRunSummary(),
                     runBlock->getStartIndex(),
                     runBlock->getLoopPeriod())) {
        return failed(executor);
    }

//    if (abs(dataPointerDelta()) != 1) {
//        // TODO: Support loops that move more than one cell per iteration
//        return failed(executor);
//    }

//    if (numBootstrapCycles() > 0) {
//        // TODO: Support loops with bootstrap
//        return failed(executor);
//    }

    _sweepValueChangeType = SweepValueChangeType::NO_CHANGE; // Initial value
    _sweepValueChange = 0;
    for (int i = numDataDeltas(); --i >= 0; ) {
        int delta = dataDeltaAt(i).delta();
        switch (_sweepValueChangeType) {
            case SweepValueChangeType::NO_CHANGE:
                _sweepValueChangeType = SweepValueChangeType::UNIFORM_CHANGE;
                _sweepValueChange = delta;
                break;
            case SweepValueChangeType::UNIFORM_CHANGE:
            case SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES:
                if (_sweepValueChange != delta) {
                    _sweepValueChangeType =
                        sign(delta) == sign(_sweepValueChange)
                        ? SweepValueChangeType::MULTIPLE_ALIGNED_CHANGES
                        : SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES;
                }
                break;
            case SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES:
                // void
                break;
        }
    }

    _exitMap.clear();
    _requiresFixedInput = false;
    for (int i = loopSize(); --i >= 0; ) {
        if (exit(i).exitWindow == ExitWindow::ANYTIME) {
            if (exitsOnZero(i)) {
                _exitMap.insert({exit(i).exitCondition.value(), i});
            } else {
                // The loop exits on a non-zero value. This means that the loop only loops when it
                // consumes a specific value.
                _requiresFixedInput = true;
            }
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const SweepLoopAnalysis& sta) {
    os << (const LoopAnalysis&)sta;
    os << "Exit values: ";
    bool isFirst = true;
    for (auto pair : sta._exitMap) {
        if (isFirst) {
            isFirst = false;
        } else {
            os << ", ";
        }
        os << pair.first << "@" << pair.second;
    }
    os << std::endl;
    if (sta._requiresFixedInput) {
        os << "Requires fixed input!" << std::endl;
    }

    return os;
}

bool SweepTransitionAnalysis::analyzeSweepTransition(int startIndex, int endIndex,
                                                     const ProgramExecutor& executor) {
    const RunSummary& runSummary = executor.getRunSummary();
    const InterpretedProgram& interpretedProgram = executor.getInterpretedProgram();

    // The instructions comprising the transition sequence
    int pbStart = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - pbStart;

    if (!analyzeSequence(interpretedProgram, runSummary, pbStart, numProgramBlocks)) {
        return failed(executor);
    }

    return true;
}

bool SweepTransitionAnalysis::transitionEquals(int startIndex, int endIndex,
                                               const ProgramExecutor& executor) const {
    const RunSummary& runSummary = executor.getRunSummary();
    const InterpretedProgram& program = executor.getInterpretedProgram();

    // The instructions comprising the transition sequence
    int pbStart = runSummary.runBlockAt(startIndex)->getStartIndex();
    int numProgramBlocks = runSummary.runBlockAt(endIndex)->getStartIndex() - pbStart;

    if (numProgramBlocks != sequenceSize()) {
        return false;
    }

    for (int i = numProgramBlocks; --i >= 0; ) {
        if (program.indexOf(programBlockAt(i)) != runSummary.programBlockIndexAt(pbStart + i)) {
            return false;
        }
    }

    return true;
}

void SweepTransitionAnalysis::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionAnalysis& sta) {
    os << (const SequenceAnalysis&)sta;

    return os;
}

void SweepTransitionGroup::clear() {
    _loops.clear();
    _transitions.clear();
}

bool SweepTransitionGroup::analyzeLoop(SweepLoopAnalysis* loopAnalysis, int runBlockIndex,
                                       const ProgramExecutor& executor) {
    const RunBlock *loopRunBlock = executor.getRunSummary().runBlockAt(runBlockIndex);

    if (!loopAnalysis->analyzeSweepLoop(loopRunBlock, executor)) {
        return false;
    }

    if (loopAnalysis->sweepValueChangeType() == SweepValueChangeType::MULTIPLE_OPPOSING_CHANGES) {
        // Not (yet?) supported
        return failed(*this);
    }

    _locatedAtRight = loopAnalysis->dataPointerDelta() > 0;
    _loops[loopRunBlock->getSequenceIndex()] = loopAnalysis;

    return true;
}

int SweepTransitionGroup::numberOfTransitionsForExitValue(int value) const {
    int count = 0;

    for (auto kv : _transitions) {
        SweepLoopExit sweepLoopExit = kv.first;
        const LoopExit &loopExit = sweepLoopExit.loop->exit(sweepLoopExit.exitInstructionIndex);

        if (loopExit.exitCondition.isTrueForValue(value)) {
            ++count;
        }
    }

    return count;
}

int SweepTransitionGroup::numberOfExitsForValue(int value) const {
    int count = 0;

    for (auto kv : _loops) {
        const SweepLoopAnalysis *loop = kv.second;
        count += loop->numberOfExitsForValue(value);
    }

    return count;
}

bool SweepTransitionGroup::determineSweepEndType() {
    bool exitToExit = false;
    bool exitToNonExit = false;
    bool nonExitToExit = false;

    for (auto kv : _transitions) {
        const SweepTransition transition = kv.second;
        const SweepTransitionAnalysis *sta = transition.transition;
        int valueAfterTransition = sta->dataDeltas().deltaAt(0);
        int finalValue = (valueAfterTransition +
                          transition.nextLoop->deltaAt( -sta->dataPointerDelta() ));
        int exitCount = numberOfExitsForValue(finalValue);

        if (exitCount > 0) {
            // The value that caused the loop to exit and continue at the given transition, is
            // converted (by the transition) to another value that causes a loop exit (in a later
            // sweep).
            exitToExit = true;

            int numTransitions = numberOfTransitionsForExitValue(finalValue);
            if (numTransitions != exitCount) {
                // Not all possible exits are (yet) followed by a transition that continues the
                // sweep. We can therefore not yet conclude this is a hang.
                return failed(*this);
            }
        } else {
            // This transition extends the sequence. It results in a value that does not cause the
            // loop to exit in a future sweep.
            exitToNonExit = true;

            // Also verify that it cannot cause the loop sweeping in the other direction to exit.
            // Note: although it was followed by a successful execution of the loop at least once
            // already, this does not proof it cannot cause that loop to exit. It could be that the
            // value was not yet seen by the instruction whose exit it can cause.
            if (_sibling->loop()->isExitValue(finalValue)) {
                return failed(*this);
            }

            int sgn = sign(finalValue);
            if (
                sgn != 0 && (
                    sgn == -sign( loop()->sweepValueChange() ) ||
                    sgn == -sign( _sibling->loop()->sweepValueChange() )
                )
            ) {
                // Although it cannot directly cause the loop to exit, it is modified by the sweeps
                // towards zero, which will likely cause it to exit the loop again.
                // TODO: Refine this check
                nonExitToExit = true;
            }
        }
    }

    // When abs(delta DP) > 1, not only change at DP is relevant. See e.g. FakeSweep.
    // TODO: Extend recognition accordingly.

    if (!exitToExit && exitToNonExit && !nonExitToExit) {
        _sweepEndType = SweepEndType::STEADY_GROWTH;
    } else if (exitToExit && !exitToNonExit) {
        _sweepEndType = SweepEndType::FIXED_POINT;
    } else if (exitToExit && exitToNonExit && !nonExitToExit) {
        _sweepEndType = SweepEndType::IRREGULAR_GROWTH;
    } else if (exitToNonExit && nonExitToExit) {
        _sweepEndType = SweepEndType::FIXED_GROWING;
    } else {
        // Unsupported sweep end type
        return false;
    }

    return true;
}

bool SweepTransitionGroup::analyzeGroup() {
//    // Check there is a transition for each anytime-exit
//    for (int i = _loop.loopSize(); --i >= 0; ) {
//        if (_loop.exit(i).exitWindow == ExitWindow::ANYTIME) {
//            if (_transitions.find(i) == _transitions.end()) {
//                // TODO: Proof that this exit can never be triggered
//                // This requires proving that the exit value can never be added to the sweep body
//                // nor to the transition appendix.
//                return failed(*this);
//            }
//        }
//    }

    if (!determineSweepEndType()) {
        return false;
    }

//    std::cout << *this << std::endl;

    int sweepDeltaSign = sign(loop()->sweepValueChange());
    _outsideDeltas.clear();
    for (auto kv : _transitions) {
        const SweepTransition transition = kv.second;
        const SweepTransitionAnalysis *analysis = transition.transition;

        int numOutside = 0;
        int maxOutsideDp = _locatedAtRight
            ? analysis->dataDeltas().maxDpOffset()
            : analysis->dataDeltas().minDpOffset();
        for (const DataDelta& dd : analysis->dataDeltas()) {
            if (dd.dpOffset() != 0) {
                bool insideSweep = (dd.dpOffset() < 0) == _locatedAtRight;
                int sgn = sign(dd.delta());
                if (insideSweep) {
                    if (sweepDeltaSign != 0 && sgn != sweepDeltaSign) {
                        // TODO: Refine or remove this check
                        return failed(*this);
                    }
                    sweepDeltaSign = sgn;
                } else {
                    switch (_sweepEndType) {
                        case SweepEndType::FIXED_POINT: {
                            // Check all deltas at specific offset have same sign
                            int delta = _outsideDeltas.deltaAt(dd.dpOffset());
                            if (delta == 0) {
                                _outsideDeltas.addDelta(dd.dpOffset(), sgn);
                            } else if (delta != sgn) {
                                return failed(*this);
                            }

                            break;
                        }
                        case SweepEndType::FIXED_GROWING:
                            if (!loop()->isExitValue(dd.delta())) {
                                return failed(*this);
                            }
                            break;
                        case SweepEndType::STEADY_GROWTH:
                            if (dd.dpOffset() != maxOutsideDp && loop()->isExitValue(dd.delta())) {
                                // The next sweep does not pass this value, which it should if the
                                // sequence is steadily growing
                                return failed(*this);
                            }
                            ++numOutside;
                            break;
                        case SweepEndType::IRREGULAR_GROWTH:
                            // Do not support this (yet?)
                            return failed(*this);
                    }
                }
            }
        }

        if (_sweepEndType == SweepEndType::STEADY_GROWTH && numOutside != abs(maxOutsideDp)) {
            // Multiple values are added beyond the DP outside the sequence, but they do not form
            // a continuous region. I.e. one or more zeroes are introduces in the sequence.
            return failed(*this);
        }
    }

    return true;
}

Trilian SweepTransitionGroup::proofHang(DataPointer dp, const Data& data) {
    switch (_sweepEndType) {
        case SweepEndType::FIXED_POINT:
            // Check all outside deltas move values away from zero
            for (const DataDelta &dd : _outsideDeltas) {
                if (dd.delta() * data.valueAt(dp, dd.dpOffset()) < 0) {
                    return Trilian::MAYBE;
                }
            }
            break;
        case SweepEndType::FIXED_GROWING: {
            int delta = locatedAtRight() ? 1 : -1;
            // Skip all appendix values
            while (true) {
                int val = data.valueAt(dp, delta);
                if (val == 0 || !loop()->isExitValue(val)) {
                    break;
                }
                dp += delta;
            }
        }
            // Fall through
        case SweepEndType::STEADY_GROWTH:
        case SweepEndType::IRREGULAR_GROWTH:
            if (!data.onlyZerosAhead(dp, locatedAtRight())) {
                return Trilian::MAYBE;
            }
            break;
    }
    return Trilian::YES;
}

std::ostream &operator<<(std::ostream &os, const SweepTransitionGroup &group) {
    for (auto kv : group._loops) {
        int sequenceIndex = kv.first;
        const SweepLoopAnalysis *loopAnalysis = kv.second;

        os << "Loop #" << sequenceIndex << std::endl;
        os << *loopAnalysis;
    }

    auto iter = group._transitions.begin();
    while (iter != group._transitions.end()) {
        SweepLoopExit loopExit = iter->first;
        SweepTransition transition = iter->second;
        os << "  Exit from Loop" << loopExit.loop->loopRunBlock()->getSequenceIndex();
        os << "@" << loopExit.exitInstructionIndex;
        os << ": " << *(transition.transition);
        os << " => Loop#" << transition.nextLoop->loopRunBlock()->getSequenceIndex() << std::endl;
        ++iter;
    }

    os << "Type = ";
    switch (group.endType()) {
        case SweepEndType::STEADY_GROWTH: os << "Steady growth"; break;
        case SweepEndType::IRREGULAR_GROWTH: os << "Irregular growth"; break;
        case SweepEndType::FIXED_POINT: os << "Fixed point"; break;
        case SweepEndType::FIXED_GROWING: os << "Fixed growing"; break;
    }

    return os;
}

SweepHangDetector::SweepHangDetector(const ProgramExecutor& executor)
: HangDetector(executor) {
    _transitionGroups[0].initSibling(&_transitionGroups[1]);
    _transitionGroups[1].initSibling(&_transitionGroups[0]);
}

int SweepHangDetector::findPrecedingTransitionStart(int sweepLoopRunBlockIndex) const {
    const RunSummary& runSummary = _executor.getRunSummary();
    int startIndex = sweepLoopRunBlockIndex;

    while (startIndex > 0) {
        int nextIndex = startIndex - 1;
        const RunBlock* runBlock = runSummary.runBlockAt(nextIndex);
        if (runBlock->isLoop()) {
            int iterations = runSummary.getRunBlockLength(nextIndex) / runBlock->getLoopPeriod();

            if (iterations > MAX_ITERATIONS_TRANSITION_LOOPS) {
                // This loop is too big to be included in the transition
                break;
            }
        }

        startIndex = nextIndex;
    }

    return startIndex;
}

bool SweepHangDetector::analyzeLoops() {
    const RunSummary& runSummary = _executor.getRunSummary();
    SweepTransitionGroup *group = _transitionGroups;

    // Assume that the loop which just finished is one of the sweep loops
    if (runSummary.getLoopIteration() < MIN_ITERATIONS_LAST_SWEEP_LOOP) {
        return failed(_executor);
    }
    int runBlockIndexLoop1 = runSummary.getNumRunBlocks() - 1;
    if (!group[1].analyzeLoop(&_loopAnalysisPool[1], runBlockIndexLoop1, _executor)) {
        return false;
    }

    int runBlockIndexTransition0 = findPrecedingTransitionStart(runBlockIndexLoop1);
    if (runBlockIndexTransition0 == 0) {
        // There is no sweep loop preceding the transition
        return failed(_executor);
    }
    int runBlockIndexLoop0 = runBlockIndexTransition0 - 1;
    if (!group[0].analyzeLoop(&_loopAnalysisPool[0], runBlockIndexLoop0, _executor)) {
        return false;
    }

    // Both loops should move in opposite directions
    if (group[0].locatedAtRight() == group[1].locatedAtRight()) {
        return failed(_executor);
    }

    auto loop0 = group[0].loop(), loop1 = group[1].loop();

    int sgn0 = sign(loop0->sweepValueChange()), sgn1 = sign(loop1->sweepValueChange());
    if (sgn1 == 0 || sgn0 == sgn1) {
        _sweepDeltaSign = sgn0;
    } else if (sgn0 == 0) {
        _sweepDeltaSign = sgn1;
    } else if (
        loop0->sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE &&
        loop1->sweepValueChangeType() == SweepValueChangeType::UNIFORM_CHANGE &&
        loop0->sweepValueChange() == -loop1->sweepValueChange()
    ) {
        _sweepDeltaSign = 0;
    } else {
        // Both loops make opposite changes that do not fully cancel out. This is not supported yet
        return failed(_executor);
    }


    if (loop0->requiresFixedInput() || loop1->requiresFixedInput()) {
        // The changes of both loops, if any, should cancel each other out.
        if (_sweepDeltaSign != 0) {
            return failed(_executor);
        }
    }

    return true;
}

bool SweepHangDetector::analyzeTransitions() {
    const RunSummary& runSummary = _executor.getRunSummary();
    const RunSummary& metaRunSummary = _executor.getMetaRunSummary();
    int metaLoopStartIndex = metaRunSummary.getLastRunBlock()->getStartIndex();
    int prevLoopIndex = runSummary.getNumRunBlocks() - 1;
    const SweepLoopAnalysis *prevLoopAnalysis = _transitionGroups[1].loop();
    int numSweeps = 0, numUniqueTransitions = 0, numUniqueLoops = 2;

    while (prevLoopIndex > metaLoopStartIndex) {
        int transitionStartIndex = findPrecedingTransitionStart(prevLoopIndex);
        if (transitionStartIndex <= metaLoopStartIndex) {
            // No more run blocks remain
            break;
        }

        int loopIndex = transitionStartIndex - 1;
        const RunBlock* loopBlock = runSummary.runBlockAt(loopIndex);
        assert(loopBlock->isLoop());

        int j = numSweeps % 2;
        SweepTransitionGroup &tg = _transitionGroups[j];
        const RunBlock* existingLoopBlock = tg.loop()->loopRunBlock();
        if (loopBlock->getSequenceIndex() != existingLoopBlock->getSequenceIndex() &&
            !runSummary.areLoopsRotationEqual(loopBlock, existingLoopBlock)
        ) {
            // Sequence does not follow expected sweep pattern anymore
            break;
        }

        const SweepLoopAnalysis* sla = tg.analysisForLoop(loopBlock);
        if (sla == nullptr) {
            // This loop was entered differently than the one(s) encountered already. Analyse it
            assert(numUniqueLoops < MAX_SWEEP_LOOP_ANALYSIS);
            SweepLoopAnalysis* newAnalysis = &_loopAnalysisPool[numUniqueLoops++];

            assert(tg.analyzeLoop(newAnalysis, loopIndex, _executor));
            sla = newAnalysis;
        }

        int loopLen = runSummary.getRunBlockLength(loopIndex);
        int exitInstruction = (loopLen - 1) % loopBlock->getLoopPeriod();
        SweepLoopExit loopExit(sla, exitInstruction);
        const SweepTransition* st = tg.transitionForExit(loopExit);
        if (st != nullptr) {
            // Check that the transition is identical
            if (!st->transition->transitionEquals(transitionStartIndex, prevLoopIndex, _executor)) {
                // Transition does not match. This may be due to start-up effects. This could still
                // be a hang.
                break;
            }
        } else {
            // This is the first transition that follows the given loop exit
            assert(numUniqueTransitions < MAX_SWEEP_TRANSITION_ANALYSIS);
            SweepTransitionAnalysis *sta = &_transitionAnalysisPool[numUniqueTransitions++];

            if (!sta->analyzeSweepTransition(transitionStartIndex, prevLoopIndex, _executor)) {
                return failed(_executor);
            }

            SweepTransition newTransition(sta, prevLoopAnalysis);
            tg.addTransitionForExit(loopExit, newTransition);
        }

        prevLoopIndex = loopIndex;
        prevLoopAnalysis = sla;
        numSweeps++;
    }

    if (numSweeps < 4) {
        // The pattern is too short
        return failed(_executor);
    }

    return true;
}

bool SweepHangDetector::analyzeTransitionGroups() {
    for (SweepTransitionGroup &tg : _transitionGroups) {
        if (!tg.analyzeGroup()) {
            return false;
        }
    }

    return true;
}

DataPointer SweepHangDetector::findAppendixStart(DataPointer dp,
                                                 const SweepTransitionGroup &group) {
    const Data& data = _executor.getData();
    int delta = group.locatedAtRight() ? -1 : 1;

    while (true) {
        int val = data.valueAt(dp, delta);
        if (val == 0 || !group.loop()->isExitValue(val)) {
            break;
        }
        dp += delta;
    }

    return dp;
}

bool SweepHangDetector::scanSweepSequence(DataPointer &dp, bool atRight) {
    const Data& data = _executor.getData();

    // For now, scan all values as the values that are skipped now may be expected during a next
    // sweep.
    // TODO?: Make analysis smarter.
    int delta = atRight ? -1 : 1;

    DataPointer dpEnd = (delta > 0) ? data.getMaxDataP() : data.getMinDataP();
    const SweepLoopAnalysis *loop0 = _transitionGroups[0].loop();
    const SweepLoopAnalysis *loop1 = _transitionGroups[1].loop();

    // DP is at one side of the sweep. Find the other end of the sweep.
    dp += delta;
    while (*dp) {
        if (loop0->isExitValue(*dp) || loop1->isExitValue(*dp)) {
            // Found end of sweep at other end
            break;
        }

        if (_sweepDeltaSign * sign(*dp) < 0) {
            // The sweep makes changes to the sequence that move some values towards zero
            return failed(_executor);
        }

        assert(dp != dpEnd); // Assumes abs(dataPointerDelta) == 1

        dp += delta;
    }

    return true;
}

bool SweepHangDetector::shouldCheckNow(bool loopContinues) {
    // Should wait for the sweep-loop to finish
    return !loopContinues && _executor.getMetaRunSummary().isInsideLoop();
}

bool SweepHangDetector::analyzeHangBehaviour() {
    const RunSummary& runSummary = _executor.getRunSummary();

    if (runSummary.getNumRunBlocks() <= 8) {
        // The run should contain two full sweeps preceded by a loop: L1 (T1 L0 T0 L1) (T1 L0 T0 L1)
        // Note, transitions are named after the loop that precedes it (as they depend on the exit
        // of that loop).
        return false;
    }

    for (SweepTransitionGroup &tg : _transitionGroups) {
        tg.clear();
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();

    if (!analyzeLoops()) {
        return false;
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();

    if (!analyzeTransitions()) {
        return false;
    }

    //((const ExhaustiveSearcher &)_executor).getProgram().dumpWeb();

    if (!analyzeTransitionGroups()) {
        return failed(_executor);
    }

//    _executor.getInterpretedProgram().dump();
//    _executor.dumpExecutionState();
//    dump();

    return true;
}

Trilian SweepHangDetector::proofHang() {
    const Data& data = _executor.getData();
    DataPointer dp1 = data.getDataPointer();

//    data.dump();

    DataPointer dp0 = dp1; // Initial value
    if (_transitionGroups[1].endType() == SweepEndType::FIXED_GROWING) {
        dp0 = findAppendixStart(dp0, _transitionGroups[1]);
    }

    if (!scanSweepSequence(dp0, _transitionGroups[1].locatedAtRight())) {
        return Trilian::MAYBE;
    }
    assert(dp0 != dp1);

    for (int i = 0; i < 2; ++i) {
        DataPointer dp = (i == 0) ? dp0 : dp1;
        Trilian result = _transitionGroups[i].proofHang(dp, data);

        if (result != Trilian::YES) {
            return result;
        }
    }

//    _executor.getRunSummary().dump();
//    _executor.getMetaRunSummary().dump();
//    dump();
//    _executor.getData().dump();
//    _executor.getInterpretedProgram().dump();

    return Trilian::YES;
}

void SweepHangDetector::dump() const {
    std::cout << *this << std::endl;
}

std::ostream &operator<<(std::ostream &os, const SweepHangDetector &detector) {
    os << "Loop #0" << std::endl;
    os << detector._transitionGroups[0] << std::endl << std::endl;

    os << "Loop #1" << std::endl;
    os << detector._transitionGroups[1] << std::endl << std::endl;

    return os;
}
