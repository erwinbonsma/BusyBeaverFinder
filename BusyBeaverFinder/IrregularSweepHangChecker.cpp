//
//  IrregularSweepHangChecker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 19/05/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "IrregularSweepHangChecker.h"

const int& (&FUN_MAX)(const int&, const int&) = std::max;
const int& (&FUN_MIN)(const int&, const int&) = std::min;

// Checks that the program execution is in a meta-meta loop consisting of:
// 1: a meta-loop (where the sweep is toggling cells inside the binary-counting appendix,
// 2: a transition (which extends the appendix)
bool IrregularSweepHangChecker::checkMetaMetaLoop(const ExecutionState& executionState) {
    auto& metaMetaRunSummary = executionState.getMetaMetaRunSummary();
    auto& metaRunSummary = executionState.getMetaRunSummary();

    if (!metaMetaRunSummary.isInsideLoop()) {
        return false;
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

        switch (location) {
            case LocationInSweep::LEFT: _endProps.insert({LocationInSweep::LEFT, {}}); break;
            case LocationInSweep::RIGHT: _endProps.insert({LocationInSweep::RIGHT, {}}); break;
            default: assert(false);
        }
    }

    return _endProps.size();
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
                assert(exit.exitCondition.getOperator() == Operator::EQUALS);

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

        if (!exitsOnZero || props.insweepExit == 0) {
            // For now, only support irregular sweep with two exits. The zero-exit extends the
            // sequence. The non-zero exit ends the sweep inside the irregular appendix.
            return false;
        }
    }

    return true;
}

bool IrregularSweepHangChecker::determineInSweepToggles() {
    for (auto& [location, props] : _endProps) {
        auto& transition = location == LocationInSweep::LEFT ? leftTransition() : rightTransition();

        // Toggle values change to in-sweep exits, but vice versa, an in-sweep exit is changed to
        // a toggle value when it caused an exit. This reverse relation is used here.
        props.insweepToggle = transition.transitionDeltas().deltaAt(0);

        auto& sweepLoop = (location == LocationInSweep::RIGHT && _rightSweepLoop
                           ? _rightSweepLoop.value()
                           : _leftSweepLoop);

        // The toggle value should change to an in-sweep exit
        auto& deltas = sweepLoop.sweepLoopDeltas();
        if (!deltas.size()) {
            // The sweep should change some values to realize an irregular sweep
            return false;
        }
        for (auto dd : deltas) {
            if (props.insweepToggle + dd.delta() != props.insweepExit) {
                // It does not (immediatly) change it to an in-sweep exit. This is not supported
                // (yet).
                return false;
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

        if (auto result = visitSweepLoopParts(visitor, executionState, 0, 0, 2); !result) {
            return false;
        } else {
            props.appendixStart = _metaLoopAnalysis->startDataPointer() + start.value();
        }
    }

    return true;
}

bool IrregularSweepHangChecker::init(const MetaLoopAnalysis* metaLoopAnalysis,
                                     const ExecutionState& executionState) {
    if (!checkMetaMetaLoop(executionState)) {
        return false;
    }

    if (!SweepHangChecker::init(metaLoopAnalysis, executionState)) {
        return false;
    }

    if (!findIrregularEnds()) {
        return false;
    }

    if (!determineInSweepExits()) {
        return false;
    }

    if (!determineInSweepToggles()) {
        return false;
    }

    if (!determineAppendixStarts(executionState)) {
        return false;
    }

    return true;
}

Trilian IrregularSweepHangChecker::proofHang(const ExecutionState& executionState) {
    return Trilian::MAYBE;
}
