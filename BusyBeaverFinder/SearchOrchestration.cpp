//
//  SearchOrchestration.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "SearchOrchestration.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "ExhaustiveSearcher.h"
#include "Utils.h"

void addInstructionsUntilTurn(std::vector<Ins> &stack, int numNoop, int numData) {
    while (numNoop-- > 0) {
        stack.push_back(Ins::NOOP);
    }
    while (numData-- > 0) {
        stack.push_back(Ins::DATA);
    }
    stack.push_back(Ins::TURN);
}

void orchestratedSearch(ExhaustiveSearcher& searcher) {
    int h = searcher.getProgram().getHeight();
    int w = searcher.getProgram().getWidth();
    std::vector<Ins> resumeStack;

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
            resumeStack.clear();
            addInstructionsUntilTurn(resumeStack, numNoop, numData);

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
                        auto sizeBefore = resumeStack.size();

                        addInstructionsUntilTurn(resumeStack, numNoop2, numData2);

                        searcher.searchSubTree(resumeStack);

                        while (resumeStack.size() > sizeBefore) {
                            resumeStack.pop_back();
                        }
                    }
                }
            } else {
                searcher.searchSubTree(resumeStack);
            }
        }
    }
}

void searchLateEscapes(ExhaustiveSearcher& searcher, std::string lateEscapesFile) {
    std::ifstream input(lateEscapesFile);
    if (!input) {
        std::cout << "Could not read file" << std::endl;
        return;
    }

    std::vector<Ins> resumeStack;
    std::string line;
    while (getline(input, line)) {
        std::istringstream iss(line);
        int numSteps;

        if (iss >> numSteps) {
            loadResumeStackFromStream(iss, resumeStack);
            if (!resumeStack.empty()) {
                searcher.searchSubTree(resumeStack, numSteps - 1);
            }
        }
    }
}
