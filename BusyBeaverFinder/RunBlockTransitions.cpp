//
//  RunBlockTransitions.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 01/04/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#include "RunBlockTransitions.h"


void RunBlockTransitions::reset() {
    _transitions.clear();
    _numTransitions = 0;
}

void RunBlockTransitions::addTransition(const RunBlock* src, const RunBlock* dst) {
    Transitions& t = _transitions[src->getSequenceId()];
    _numTransitions += 1;

    auto transitionEntry = t.destNodes.find(dst->getSequenceId());
    if (transitionEntry != t.destNodes.end()) {
        // A known transition. Updates its occurence trace.
        transitionEntry->second.lastOccurence = _numTransitions;
        transitionEntry->second.count += 1;
        return;
    }

    // Add new transition
    t.destNodes.emplace(std::piecewise_construct,
                        std::forward_as_tuple(dst->getSequenceId()),
                        std::forward_as_tuple(_numTransitions));

    if (t.destNodes.size() > _maxDestinationsPerNode) {
        // Exceeded the maximum number of transitions. Drop the oldest

        int min = _numTransitions;
        auto minIt = t.destNodes.end();
        for (auto it = t.destNodes.begin(), end = t.destNodes.end(); it != end; ++it) {
            if (it->second.lastOccurence < min) {
                min = it->second.lastOccurence;
                minIt = it;
            }
        }
        t.destNodes.erase(minIt);
    }
}


void RunBlockTransitions::processNewRunBlocks() {
    int target = _runSummary.getNumRunBlocks() - 1;
    const RunBlock* prv = _runSummary.runBlockAt(_numTransitions);
    while (_numTransitions < target) {
        const RunBlock* cur = _runSummary.runBlockAt(_numTransitions + 1);
        addTransition(prv, cur);
        prv = cur;
    }
}

void RunBlockTransitions::dump() const {
    for (auto& t : _transitions) {
        std::cout << "From " << t.first << " to: ";
        bool isFirst = true;
        for (auto& dest : t.second.destNodes) {
            if (!isFirst) {
                std::cout << ", ";
            }
            isFirst = false;
            std::cout << dest.first
                << "(# = " << dest.second.count << ", t = " << dest.second.lastOccurence << ")";
        }
        std::cout << std::endl;
    }
}
