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

Ins* SearchOrchestrator::addInstructionsUntilTurn(Ins* topP, int numNoop, int numData) {
    while (numNoop-- > 0) {
        *(topP++) = Ins::NOOP;
    }
    while (numData-- > 0) {
        *(topP++) = Ins::DATA;
    }
    *(topP++) = Ins::TURN;

    return topP;
}

void SearchOrchestrator::search() {
    int h = _searcher.getProgram().getHeight();
    int w = _searcher.getProgram().getWidth();
    int maxInstructions = (h + 1 > w + 2) ? h + 1 : w + 2;
    Ins* startFrom = new Ins[maxInstructions];

    // Only the number of DATA instructions before the first TURN matters, not their position, as
    // they will only be visited once at the very start of the program (and possibly as final
    // instruction before termination). So there is no need to iterate over all possible
    // permutations of DATA and NOOP instructions. Therefore only the canonical variant is searched.
    //
    // y = The (row) position of first TURN instructions that is encountered.
    // n = The number of DATA instructions preceding the first TURN
    //
    // For the program to not terminate immediately, at least one DATA instruction should precede
    // the first TURN. Therefore, y > 0 and n > 0.
    for (int y = 1; y < h; y++) {
        for (int n = 1; n <= y; n++ ) {
            int numData = n;
            int numNoop = y - numData;

            Ins* topP = addInstructionsUntilTurn(startFrom, numNoop, numData);

            if (y == 1) {
                // When the first TURN is at the second row, the program pointer traverses the first
                // row. Here again, only the number of DATA instructions before the second TURN
                // matters, not their order.
                //
                // x = The column position of the second TURN instruction
                // m = The number of DATA instructions preceding this second TURN
                for (int x = 2; x < w; x++) {
                    for (int m = 1; m < x; m++) {
                        numData = m;
                        numNoop = x - numData - 1;
                        Ins* topP2 = addInstructionsUntilTurn(topP, numNoop, numData);

                        (*topP2) = Ins::UNSET;
                        _searcher.searchSubTree(startFrom);
                    }
                }
            } else {
                (*topP) = Ins::UNSET;
                _searcher.searchSubTree(startFrom);
            }
        }
    }

    delete[] startFrom;
}
