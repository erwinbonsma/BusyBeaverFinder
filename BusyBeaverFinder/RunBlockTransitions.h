//
//  RunBlockTransitions.h
//  BusyBeaverFinder
//
//  Created by Erwin on 01/04/2024.
//  Copyright © 2024 Erwin. All rights reserved.
//

#ifndef RunBlockTransitions_h
#define RunBlockTransitions_h

#include <map>

#include "RunSummary.h"

class RunBlockTransitions {

    // The maximum number of destinations that should be maintained per node.
    // When the maximum would be exceeded, the oldest transition is dropped.
    size_t _maxDestinationsPerNode { 4 };

    struct Transition {
        Transition(size_t lastOccurence) : count(1), lastOccurence(lastOccurence) {}

        size_t count = 0;
        size_t lastOccurence = 0;
    };

    struct Transitions {
        // Map with key sequenceId of destination node and value index (in run summary) when
        // transition last occured
        std::map<size_t, Transition> destNodes;

    };

    // Map with key sequenceId of source node
    std::map<size_t, Transitions> _transitions;
    size_t _numTransitions = 0;

    const RunSummary& _runSummary;

    void addTransition(const RunBlock* from, const RunBlock* to);

public:
    RunBlockTransitions(const RunSummary& runSummary) : _runSummary(runSummary) {}

    void setMaxDestinationsPerNode(size_t value) { _maxDestinationsPerNode = value; }

    void reset();
    void processNewRunBlocks();

    void dump() const;
};

#endif /* RunBlockTransitions_h */
