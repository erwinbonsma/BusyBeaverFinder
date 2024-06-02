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
// 1: a meta-loop (where the sweep is toggling cells inside the binary-counting appendix,
// 2: a transition (which extends the appendix)
//
// Note, also succeeds when the program is not in a meta-meta loop, as there are irregular
// sweeps that are stuck in a meta-loop. This is the case when there in-sweep exit is the same
// as the sweep extending exit.
bool IrregularSweepHangChecker::checkMetaMetaLoop(const ExecutionState& executionState) {
    auto& metaMetaRunSummary = executionState.getMetaMetaRunSummary();
    auto& metaRunSummary = executionState.getMetaRunSummary();

    _isInsideMetaMetaLoop = metaMetaRunSummary.isInsideLoop();
    if (!_isInsideMetaMetaLoop) {
        // Irregular sweeps with a zero in-sweep exit do not end up in a meta-meta loop.
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

    _endProps.clear();
    for (auto [location, counts] : map) {
        if (!counts.second) {
            continue;
        }

        if (counts.second != counts.first) {
            // All sweeps to the same location should be irregular
            return false;
        }

        _irregularEnd = location;
        _endProps.insert({location, {}});
    }

    // Do not (yet) supports sweeps where both ends are irregular. Doing so would complicate
    // proofHang checks.
    return _endProps.size() == 1;
}

bool IrregularSweepHangChecker::determineInSweepExits() {
    for (auto& [location, props] : _endProps) {
        auto& incomingSweeps = (location == LocationInSweep::LEFT ? leftSweepLoop().incomingLoops()
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
    }

    return true;
}

bool IrregularSweepHangChecker::determineInSweepToggles() {
    for (auto& [location, props] : _endProps) {
        auto& transition = location == LocationInSweep::LEFT ? leftTransition() : rightTransition();

        // Toggle values change to in-sweep exits, but vice versa, an in-sweep exit is changed to
        // a toggle value when it caused an exit. This reverse relation is used here.
        props.insweepToggle = props.insweepExit + transition.transitionDeltas().deltaAt(0);

        auto& sweepLoop = (location == LocationInSweep::RIGHT && _rightSweepLoop
                           ? _rightSweepLoop.value()
                           : _leftSweepLoop);

        // The toggle value should change to an in-sweep exit
        auto& deltas = sweepLoop.sweepLoopDeltas();
        if (deltas.size()) {
            // Case 1: The sweep loop should change toggle values to in-sweep exits
            for (auto dd : deltas) {
                if (props.insweepToggle + dd.delta() != props.insweepExit) {
                    // It does not (immediatly) change it to an in-sweep exit. This is not
                    // supported (yet).
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
                if (dd.dpOffset() != 0 && props.insweepToggle + dd.delta() != props.insweepExit) {
                    // It does not (immediatly) change it to an in-sweep exit. This is not
                    // supported (yet).
                    return false;
                }
            }
        }
    }

    return true;
}

bool IrregularSweepHangChecker::determineAppendixStarts(const ExecutionState& executionState) {
    for (auto& [location, props] : _endProps) {
        std::optional<int> start;
        auto targetLocation = location;
        auto &cmp = location == LocationInSweep::LEFT ? FUN_MAX : FUN_MIN;

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
    }

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

    if (!determineInSweepExits()) {
        return false;
    }

//    std::cout << *this;

    if (!determineInSweepToggles()) {
        return false;
    }

    if (!determineAppendixStarts(executionState)) {
        return false;
    }

    return true;
}

bool IrregularSweepHangChecker::sweepLoopContinuesForever(const ExecutionState& executionState,
                                                          SweepLoop* loop, int seqIndex) {
    bool departsFromIrregularEnd {};
    if (_irregularEnd == LocationInSweep::LEFT) {
        if (loop == &_leftSweepLoop) {
            departsFromIrregularEnd = true;
        } else {
            assert(_midTransition);

            // There is a mid-sweep transition, the irregular end is at the left, and this loop
            // departs from the right, so this is a regular sweep
            return SweepHangChecker::sweepLoopContinuesForever(executionState, loop, seqIndex);
        }
    } else {
        assert(_irregularEnd == LocationInSweep::RIGHT);

        if (loop == &_leftSweepLoop) {
            if (_rightSweepLoop) {
                assert(_midTransition);

                // There is a mid-sweep transition, the irregular end is at the right, and this
                // loop departs from the left, so this is a regular sweep
                return SweepHangChecker::sweepLoopContinuesForever(executionState, loop, seqIndex);
            } else {
                // There is no mid-sweep transition; we are moving towards the irregular end at the
                // right
                departsFromIrregularEnd = false;
            }
        } else {
            // There is a mid-sweep transition, and we are moving from the irregular end at the
            // right towards it.
            departsFromIrregularEnd = true;
        }
    }

    // TODO:
    // Check that sweep over body continues forever

    return true;
}

bool IrregularSweepHangChecker::transitionContinuesForever(const ExecutionState& executionState,
                                                           TransitionGroup* transition,
                                                           int seqIndex) {
    bool checkAppendix = false;
    bool atLeft = _irregularEnd == LocationInSweep::LEFT;

    if (atLeft) {
        if (transition == &_leftTransition) {
            checkAppendix = true;
        }
    } else {
        if (transition == &_rightTransition) {
            checkAppendix = true;
        }
    }

    if (!checkAppendix) {
        // Use regular transition check
        return SweepHangChecker::transitionContinuesForever(executionState, transition, seqIndex);
    }

    // Check the appendix. It should consist of only in-sweep exits, in-sweep toggles and
    // pollution that moves away from zero. Beyond the appendix there should only be zeroes ahead.
    const Data& data = executionState.getData();

    auto &dpCmpFun = atLeft ? FUN_MAX_PTR : FUN_MIN_PTR;
    int dpDelta = atLeft ? -1 : 1;
    DataPointer dpLimit = atLeft ? data.getMinBoundP() : data.getMaxBoundP();
    auto &props = (atLeft
                   ? _endProps.at(LocationInSweep::LEFT)
                   : _endProps.at(LocationInSweep::RIGHT));
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
                    // Pollution should move away from zero
                    return false;
                }
            }
        }

        dp += dpDelta;
    }

    return true;
}
