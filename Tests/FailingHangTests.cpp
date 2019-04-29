//
//  FailingHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "6x6 Failing Hang tests", "[hang][6x6][.fail]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 1000000;
    settings.maxHangDetectAttempts = 1024;
    searcher.configure(settings);

    SECTION( "6x6-IrregularSweep3") {
        // Another irregular sweep.
        //
        // It has a meta-meta-run loop.
        // The meta-run is as follows: #10(9) #9[12 14 7 16 ...]
        // The sweep loops are 12 and 7. Meta-loop #9 also has an increasing number of iterations.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        // * _ o o *
        // * _ o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::IRREGULAR_SWEEP) == 1);
    }
}
