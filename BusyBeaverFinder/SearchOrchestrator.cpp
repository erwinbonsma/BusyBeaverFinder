//
//  SearchOrchestrator.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "SearchOrchestrator.h"

#include "ExhaustiveSearcher.h"

SearchOrchestrator::SearchOrchestrator(ExhaustiveSearcher& searcher)
    : _searcher(searcher) {
    // void
}

void SearchOrchestrator::search() {
    int h = _searcher.getProgram().getHeight();
    Op* startFrom = new Op[h + 1];

    // Only the number of DATA instructions before the first TURN matters, not their position, as
    // they will only be visited once at the very start of the program (and possibly as final
    // instruction before termination). So there is no need to iterate over all possible
    // permutations of DATA and NOOP instructions. Therefore only the canonical variant is searched.
    //
    // y = The position of first TURN instructions that is encountered.
    // n = The number of DATA instructions encountered before the first TURN
    //
    // For the program to not terminate immediately, at least one DATA instruction should precede
    // the first TURN. Therefore, y > 0 and n > 0.
    for (int y = 1; y < h; y++) {
        for (int n = 1; n <= y; n++ ) {
            int numData = n;
            int numNoop = y - numData;
            int i = 0;
            while (numNoop-- > 0) {
                startFrom[i++] = Op::NOOP;
            }
            while (numData-- > 0) {
                startFrom[i++] = Op::DATA;
            }
            startFrom[i++] = Op::TURN;
            startFrom[i++] = Op::UNSET;

            _searcher.searchSubTree(startFrom);
        }
    }

    delete[] startFrom;
}
