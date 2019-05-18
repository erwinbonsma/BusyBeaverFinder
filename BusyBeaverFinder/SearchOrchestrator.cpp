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
    // numNoop = The number of NOOP instructions before the first TURN
    // numData = The number of DATA instructions before the first TURN
    //
    // For the program to not terminate immediately, at least one DATA instruction should precede
    // the first TURN. Therefore: numData > 0
    for (int numNoop = h - 1; --numNoop >= 0; ) {
        for (int numData = h - numNoop; --numData >= 1; ) {
            Ins* topP = addInstructionsUntilTurn(startFrom, numNoop, numData);

            if (numData == 1 && numNoop == 0) {
                // When the first TURN is at the second row, the program pointer traverses the first
                // row. Here again, only the number of DATA instructions before the second TURN
                // matters, not their order.
                //
                // numNoop2 = The number of NOOP instructions before the second TURN
                // numData2 = The number of DATA instructions before the second TURN. It excludes
                //            the single data instruction before the first TURN
                for (int numNoop2 = w - 1; --numNoop2 >= 0; ) {
                    for (int numData2 = w - numNoop2 - 1; --numData2 >= 1; ) {
                        Ins* topP2 = addInstructionsUntilTurn(topP, numNoop2, numData2);

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
