//
//  InterpretationTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 08/05/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <stdio.h>

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "7x7 Interpretation Tests", "[interpretation][7x7]" ) {
    SearchSettings settings = defaultSearchSettings;
    ExhaustiveSearcher searcher(7, 7, settings);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SECTION( "ProgramBlockWithManyEntries" ) {
        // An interpreted program with a program block that has eight entries. This is the non-zero
        // block that starts at the selected program cell.
        //
        //   * * *   *
        // * _ o[o]* _
        //   * o _ o o *
        // * o o o _ *
        //   _ * _ o _ *
        // * _ o _ *
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        // TODO: Add actual test. For now, it suffices that the program does not throw an assertion
        // failure because there are too many entries.
        REQUIRE(true);
    }
}
