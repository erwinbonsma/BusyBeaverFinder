//
//  IrregularSweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/05/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "IrregularSweepHangChecker.h"
#include "Data.h"

using CompFun = const int&(const int&, const int&);
CompFun &FUN_MAX = std::max;
CompFun &FUN_MIN = std::min;

using CompFunPtr = const int *const &(const int *const &, const int *const &);
CompFunPtr &FUN_MAX_PTR = std::max;
CompFunPtr &FUN_MIN_PTR = std::min;

// Checks if the program execution is in a meta-meta loop. If it is, it should consist of:
// 1: a meta-loop (where the sweep is toggling cells inside the binary-counting appendix), and
// 2: a transition (which extends the appendix)
//
// Note, also succeeds when the program is not in a meta-meta loop, as there are irregular
// sweeps that are stuck in a meta-loop. This is the case when:
// 1. the in-sweep exit is the same as the sweep extending exit, or
// 2. the sweep shrinks irregularly, but controlled by a counter whose start value increases over
//    time.
bool IrregularSweepHangChecker::checkMetaMetaLoop(const ExecutionState& executionState) {
    auto& metaMetaRunSummary = executionState.getMetaMetaRunSummary();
    auto& metaRunSummary = executionState.getMetaRunSummary();

    _isInsideMetaMetaLoop = metaMetaRunSummary.isInsideLoop();
    if (!_isInsideMetaMetaLoop) {
        // Not all irregular sweeps end up in a meta-meta loop.
        return true;
    }

    auto metaMetaRunBlock = metaMetaRunSummary.getLastRunBlock();
    constexpr int ExpectedMetaMetaLoopPeriod = 2;
    if (metaMetaRunBlock->getLoopPeriod() != ExpectedMetaMetaLoopPeriod) {
        return false;
    }

    int numMetaLoops = 0;
    for (int i = 0; i < ExpectedMetaMetaLoopPeriod; ++i) {
        int rbIndex = metaMetaRunBlock->getStartIndex() + i;
        auto metaRunBlock = metaRunSummary.runBlockAt(rbIndex);
        if (metaRunBlock->isLoop()) {
            numMetaLoops += 1;
        } else {
            if (metaRunSummary.getRunBlockLength(rbIndex) > 2) {
                // The appendix extending transition should be plain. It should contain at most
                // one loop (e.g. a different return sweep that behaves similarly to the normal
                // one)
                return false;
            }
        }
    }
    if (numMetaLoops != 1) {
        return false;
    }

    return true;
}

bool IrregularSweepHangChecker::findIrregularEnds() {
    // Tracks for each sweep location:
    // 1: How many sweeps end there, 2: How many sweep ends are irregular
    std::map<LocationInSweep, std::pair<int, int>> map;

    size_t loopSize = _metaLoopAnalysis->loopSize();
    for (int i = 0; i < loopSize; ++i) {
        auto& location = locationInSweep(i);
        if (!location.isSweepLoop()) {
            continue;
        }

        int loopIndex = _metaLoopAnalysis->loopIndexForSequence(i);
        auto& behavior = _metaLoopAnalysis->loopBehaviors().at(loopIndex);

        auto& entry = map[location.end];
        entry.first += 1;

        bool endIsIrregular = !behavior.endDpGrowth();
        if (!endIsIrregular) {
            continue;
        }

        if (endIsIrregular && location.end == LocationInSweep::MID) {
            // This should not happen
            return false;
        }
        entry.second += 1;
    }

    _irregularEnds.clear();
    for (auto [location, counts] : map) {
        if (!counts.second) {
            continue;
        }

        if (counts.second != counts.first) {
            // All sweeps to the same location should be irregular
            return false;
        }

        _irregularEnds.push_back(location);
    }

    if (_irregularEnds.size() != 1) {
        // Do not (yet) support sweeps where both ends are irregular. Doing so would complicate
        // proofHang checks.
        return false;
    }

    _irregularEnd = _irregularEnds[0];

    return true;
}

bool IrregularSweepHangChecker::determineInSweepExits(IrregularAppendixProps& props) {
    auto& incomingSweeps = (props.location == LocationInSweep::LEFT
                            ? leftSweepLoop().incomingLoops()
                            : rightSweepLoop() ? rightSweepLoop().value().incomingLoops()
                                               : leftSweepLoop().outgoingLoops());
    bool exitsOnZero = false;
    for (auto& analysis : incomingSweeps) {
        for (auto& exit : analysis->loopExits()) {
            if (exit.exitWindow != ExitWindow::ANYTIME) {
                continue;
            }

            // The <= and >= operators only apply for stationary loops.
            // The != cannot apply for an irregular sweep loop, as its a solitary exit.
            if (exit.exitCondition.getOperator() != Operator::EQUALS) {
                return false;
            }

            if (exit.exitCondition.value() == 0) {
                exitsOnZero = true;
            } else {
                if (props.insweepExit != 0) {
                    // For now, only support sweep loops with a single non-zero exit
                    return false;
                }
                props.insweepExit = exit.exitCondition.value();
            }
        }
    }

    if (!exitsOnZero) {
        // The sweep always should stop when there are only zeroes ahead.
        return false;
    }

    if (_isInsideMetaMetaLoop) {
        if (props.insweepExit == 0) {
            // There should a non-zero insweep exit inside the irregular appendix.
            return false;
        }
    } else {
        if (props.insweepExit != 0) {
            // Programs not in a meta-meta loop should have an insweep exit of zero (so that
            // the execution paths of the insweep turn and appendix extension are the same)
            return false;
        }
    }

    return true;
}

bool IrregularSweepHangChecker::determineInSweepToggles(IrregularAppendixProps& props) {
    auto& transition = (props.location == LocationInSweep::LEFT
                        ? leftTransition()
                        : rightTransition());

    // Toggle values change to in-sweep exits, but vice versa, an in-sweep exit is changed to
    // a toggle value when it caused an exit. This reverse relation is used here.
    props.insweepToggle = props.insweepExit + transition.transitionDeltas().deltaAt(0);

    auto& sweepLoop = (props.location == LocationInSweep::RIGHT && _rightSweepLoop
                       ? _rightSweepLoop.value()
                       : _leftSweepLoop);

    // The toggle value should change to an in-sweep exit
    auto& deltas = sweepLoop.sweepLoopDeltas();
    props.insweepDelta = 0;
    if (deltas.size()) {
        // Case 1: The sweep loop should change toggle values to in-sweep exits
        for (auto dd : deltas) {
            if (!props.insweepDelta) {
                props.insweepDelta = dd.delta();
            } else if (props.insweepDelta != dd.delta()) {
                // There should only be a single delta value. This ensures that each toggle
                // value eventually changes into an in-sweep exit
                return false;
            }
        }

        // The following should hold: insweepToggle + n * delta = insweepExit, with n > 0
        int diff = props.insweepExit - props.insweepToggle;
        if (diff % props.insweepDelta != 0 || diff * props.insweepDelta < 0) {
            // Repeated addition of delta to toggle does not result in an insweep exit
            return false;
        }

        // Any values that the transition modifies next to the exit should apply the same in-sweep
        // delta. Otherwise there can be false positive detections (e.g. for BB 7x7 #117273)
        auto& transDeltas = transition.transitionDeltas();
        for (auto dd : transDeltas) {
            if (dd.dpOffset() != 0 && dd.delta() != props.insweepDelta) {
                return false;
            }
        }
    } else {
        // Case 2: The transition sequence changes a toggle value to an in-sweep exit.
        auto& deltas = transition.transitionDeltas();
        if (deltas.size() < 2) {
            // There should be at least two deltas. One that changes the current in-sweep exit
            // to a toggle, and a second that changes a toggle value to an in-sweep exit.
            return false;
        }
        for (auto dd : deltas) {
            if (dd.dpOffset() == 0) {
                continue;
            }
            if (props.insweepToggle + dd.delta() != props.insweepExit) {
                // It does not (immediatly) change it to an in-sweep exit. This is not
                // supported (yet).
                return false;
            }
        }

        props.insweepDelta = props.insweepExit - props.insweepToggle;
    }

    return true;
}

bool IrregularSweepHangChecker::determineAppendixStarts(IrregularAppendixProps& props,
                                                        const ExecutionState& executionState) {
    std::optional<int> start {};
    auto targetLocation = props.location;
    auto &cmp = props.location == LocationInSweep::LEFT ? FUN_MAX : FUN_MIN;

    auto visitor = [targetLocation, &start, &cmp, this](const SweepLoopVisitState& vs) {
        auto &loc = locationInSweep(vs.loopPart.seqIndex());
        if (loc.start == targetLocation) {
            if (!start) {
                start = vs.loopPart.dpOffset();
            } else {
                start = cmp(start.value(), vs.loopPart.dpOffset());
            }
        }

        return true;
    };

    SweepVisitOptions options = { .numLoopIterations = 2 };
    if (auto result = visitSweepLoopParts(visitor, executionState, 0, options); !result) {
        return false;
    } else {
        props.appendixStart = _metaLoopAnalysis->startDataPointer() + start.value();
    }

    return true;
}

bool IrregularSweepHangChecker::checkForAppendix(LocationInSweep location,
                                                 const ExecutionState& executionState) {
    IrregularAppendixProps props { location };

    if (!determineInSweepExits(props)) {
        return false;
    }

    if (!determineInSweepToggles(props)) {
        return false;
    }

    if (!determineAppendixStarts(props, executionState)) {
        return false;
    }

    _endProps.insert({ location, props });

    return true;
}

bool IrregularSweepHangChecker::determineLoopExitValue(ShrinkingEndProps& props) {
    auto& incomingSweeps = (props.location == LocationInSweep::LEFT
                            ? leftSweepLoop().incomingLoops()
                            : rightSweepLoop() ? rightSweepLoop().value().incomingLoops()
                            : leftSweepLoop().outgoingLoops());

    std::optional<int> loopExit {};
    for (auto& analysis : incomingSweeps) {
        // Check that the loop exits on zero
        for (auto& exit : analysis->loopExits()) {
            if (exit.exitWindow != ExitWindow::ANYTIME) {
                continue;
            }

            if (exit.exitCondition.getOperator() != Operator::EQUALS) {
                return false;
            }

            if (exit.exitCondition.value() == 0) {
                // The loop should never reach a zero. It should end when the non-zero counter
                // becomes zero
                return false;
            }

            if (!loopExit) {
                loopExit = exit.exitCondition.value();
            } else {
                if (loopExit.value() != exit.exitCondition.value()) {
                    // Only support a single delta for the loop counter
                    return false;
                }
            }
        }
    }

    if (!loopExit) {
        return false;
    }

    props.loopExit = loopExit.value();

    return true;
}

bool IrregularSweepHangChecker::determineCounterDelta(ShrinkingEndProps& props) {
    auto& transition = (props.location == LocationInSweep::LEFT
                        ? leftTransition()
                        : rightTransition());

    // Check that the following holds:
    // 1. The loop exit remains unchanged (it does not cause the sweep to move)
    // 2. There is an in-sweep delta that moves a counter towards zero that eventually becomes the
    //    new loop exit.
    // 3. There is another in-sweep delta, even more inside the sweep, that increments the next
    //    counter.

    auto& deltas = transition.transitionDeltas();
    bool atLeft = props.location == LocationInSweep::LEFT;
    std::optional<DataDelta> curDelta;
    std::optional<DataDelta> nxtDelta;

    for (auto dd : deltas) {
        if (dd.dpOffset() == 0) {
            // The loop exit should remain unchanged
            return false;
        }

        bool inside = atLeft == (dd.dpOffset() > 0);
        if (inside) {
            if (!curDelta) {
                curDelta = dd;
            } else if (!nxtDelta) {
                if (atLeft == (dd.dpOffset() > curDelta.value().dpOffset())) {
                    nxtDelta = dd;
                } else {
                    nxtDelta = curDelta;
                    curDelta = dd;
                }
            } else {
                // Only support two in-sweep deltas
                return false;
            }
        } else {
            // The values outside the sweep should move away from zero
            if ((props.loopExit > 0) != (dd.delta() > 0)) {
                return false;
            }
        }
    }

    if (!nxtDelta) {
        // There should be two in-sweep deltas
        return false;
    }

    if (nxtDelta.value().delta() * curDelta.value().delta() >= 0) {
        // The values should have opposite signs
        return false;
    }

    if (abs(curDelta.value().delta()) != 1) {
        // For now only support decrementing the counter by one. This ensures that the counter
        // will reach zero (as long as its sign is correct).
        return false;
    }

    props.counterIsPositive = curDelta.value().delta() > 0;

    return true;
}

bool IrregularSweepHangChecker::checkForShrinkage(LocationInSweep location) {
    ShrinkingEndProps props { location };

    if (!determineCounterDelta(props)) {
        return false;
    }

    if (!determineCounterDelta(props)) {
        return false;
    }

    auto& oppTransition = location == LocationInSweep::LEFT ? rightTransition() : leftTransition();
    if (oppTransition.isStationary()) {
        // The sweep needs to grow at the other side for the program to hang
        return false;
    }

    _endProps.insert({ location, props });

    return true;
}

bool IrregularSweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                                     const ExecutionState& executionState) {
//    executionState.dumpExecutionState();

    if (!checkMetaMetaLoop(executionState)) {
        return false;
    }

    if (!SweepHangChecker::init(metaLoopAnalysis, executionState)) {
        return false;
    }

    if (!findIrregularEnds()) {
        return false;
    }

    // Redo transition analysis of irregular end with forced stationary analysis, to ensure it is
    // analyzed correctly.
    if (_irregularEnd == LocationInSweep::LEFT) {
        if (!_leftTransition.analyze(*this, executionState, true)) {
            return false;
        }
    } else {
        if (!_rightTransition.analyze(*this, executionState, true)) {
            return false;
        }
    }

    _endProps.clear();
    for (auto location : _irregularEnds) {
        if (!checkForAppendix(location, executionState) && !checkForShrinkage(location)) {
            return false;
        }
    }

    return true;
}

bool IrregularSweepHangChecker::sweepLoopContinuesForever(const ExecutionState& executionState,
                                                          SweepLoop* loop, int seqIndex) {
//    bool departsFromIrregularEnd {};
//    if (_irregularEnd == LocationInSweep::LEFT) {
//        if (loop == &_leftSweepLoop) {
//            departsFromIrregularEnd = true;
//        } else {
//            assert(_midTransition);
//
//            // There is a mid-sweep transition, the irregular end is at the left, and this loop
//            // departs from the right, so this is a regular sweep
//            return SweepHangChecker::sweepLoopContinuesForever(executionState, loop, seqIndex);
//        }
//    } else {
//        assert(_irregularEnd == LocationInSweep::RIGHT);
//
//        if (loop == &_leftSweepLoop) {
//            if (_rightSweepLoop) {
//                assert(_midTransition);
//
//                // There is a mid-sweep transition, the irregular end is at the right, and this
//                // loop departs from the left, so this is a regular sweep
//                return SweepHangChecker::sweepLoopContinuesForever(executionState, loop, seqIndex);
//            } else {
//                // There is no mid-sweep transition; we are moving towards the irregular end at the
//                // right
//                departsFromIrregularEnd = false;
//            }
//        } else {
//            // There is a mid-sweep transition, and we are moving from the irregular end at the
//            // right towards it.
//            departsFromIrregularEnd = true;
//        }
//    }
    auto &lb = loopBehavior(seqIndex);
    if (lb.iterationDeltaType() == LoopIterationDeltaType::LINEAR_INCREASE) {
        // This is a regular sweep. This happens when it departs from the regular end of the sweep
        // and terminates at a mid-sweep transition.
        return SweepHangChecker::sweepLoopContinuesForever(executionState, loop, seqIndex);
    }

    // This is a sweep-loop that either departs from an irregular end, or reaches it.

    // TODO:
    // Check that sweep over body continues forever

    return true;
}

bool IrregularSweepHangChecker::growingTransitionContinuesForever
    (const ExecutionState& executionState, TransitionGroup* transition, int seqIndex) {

    bool atLeft = _irregularEnd == LocationInSweep::LEFT;

    // Check the appendix. It should consist of only in-sweep exits, in-sweep toggles and
    // pollution that moves away from zero. Beyond the appendix there should only be zeroes ahead.
    const Data& data = executionState.getData();

    auto &dpCmpFun = atLeft ? FUN_MAX_PTR : FUN_MIN_PTR;
    int dpDelta = atLeft ? -1 : 1;
    DataPointer dpLimit = atLeft ? data.getMinBoundP() : data.getMaxBoundP();
    auto &props = std::get<IrregularAppendixProps>(_endProps.at(_irregularEnd));
    DataPointer appendixStart = props.appendixStart;
    auto &dvCmpFun = props.insweepToggle > props.insweepExit ? FUN_MIN : FUN_MAX;

    if (dpCmpFun(data.getDataPointer(), appendixStart) != appendixStart) {
        // DP should be "beyond" the appendix start
        return false;
    }

    DataPointer dp = appendixStart;
    while (dpCmpFun(dp, dpLimit) == dp) {
        if (*dp != props.insweepExit && *dp != props.insweepToggle) {
            if (*dp == 0) {
                if (dp != data.getDataPointer()) {
                    // There should not be any zeroes in between non-zero values. The exception is
                    // the current in-sweep exit value which just became zero.
                    return false;
                }
            } else {
                if (dvCmpFun(*dp, 0) == 0) {
                    // This value moves towards zero. It should change into an insweep exit.
                    //
                    // This check ensures that the number of active values in the appendix does not
                    // decrease. Programs may initially fail this check, but can pass it when this
                    // limited set of values has become pollution that moves away from zero.
                    if ((*dp - props.insweepExit) % props.insweepDelta != 0) {
                        return false;
                    }
                } else {
                    // The appendix is polluted with values that move away from zero. That is okay.
                }
            }
        }

        dp += dpDelta;
    }

    return true;
}

bool IrregularSweepHangChecker::shrinkingTransitionContinuesForever
    (const ExecutionState& executionState, TransitionGroup* transition, int seqIndex) {

    // TODO

    return true;
}

bool IrregularSweepHangChecker::transitionContinuesForever(const ExecutionState& executionState,
                                                           TransitionGroup* transition,
                                                           int seqIndex) {
    bool atIrregularEnd = false;
    bool atLeft = _irregularEnd == LocationInSweep::LEFT;

    if (atLeft) {
        if (transition == &_leftTransition) {
            atIrregularEnd = true;
        }
    } else {
        if (transition == &_rightTransition) {
            atIrregularEnd = true;
        }
    }

    if (!atIrregularEnd) {
        // Use regular transition check
        return SweepHangChecker::transitionContinuesForever(executionState, transition, seqIndex);
    }

    if (std::holds_alternative<IrregularAppendixProps>(_endProps.at(_irregularEnd))) {
        return growingTransitionContinuesForever(executionState, transition, seqIndex);
    } else {
        return shrinkingTransitionContinuesForever(executionState, transition, seqIndex);
    }
}
