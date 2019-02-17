//
//  SearchOrchestrator.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef SearchOrchestrator_h
#define SearchOrchestrator_h

#include <stdio.h>

#include "Types.h"

class ExhaustiveSearcher;

class SearchOrchestrator {
    ExhaustiveSearcher& _searcher;

    // Adds instructions to the instruction stack where topP points to the top of the stack. It
    // first adds numNoop NOOP instructions, then numData DATA instructions followed by one turn
    // instruction. It returns the pointer to the new top of the stack.
    Ins* addInstructionsUntilTurn(Ins* topP, int numNoop, int numData);

public:
    SearchOrchestrator(ExhaustiveSearcher& searcher);

    void search();
};

#endif /* SearchOrchestrator_h */
