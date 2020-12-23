//
//  LateEscapeFollowUpTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 26/05/19.
//  Copyright © 2019 Erwin Bonsma
//

#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "7x7 Late Escape Follow-Up tests", "[7x7][late-escape]" ) {
    ExhaustiveSearcher searcher(7, 7, 16384);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxSteps = 10000000;
    searcher.configure(settings);

    SECTION( "EscapeIntoSuccess" ) {
        // After "escape" terminates without encountering unset instructions. This is therefore
        // not an actual late escape.
        //
        //   *     * *
        // * o o _ _ _ *
        //   _ * * o _
        // * o _ o o *
        // o _ o o o *
        // o * _ _ o *
        // o       *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };

        searcher.searchSubTree(resumeFrom, true);

        REQUIRE(tracker.getTotalSuccess() == 1);
        REQUIRE(tracker.getMaxStepsFound() == 3152126);
    }
    SECTION( "EscapeIntoSearch" ) {
        // After 6326 steps, does not encounter any new instructions until program escapes after
        // 1648530 steps.
        //
        // After escape encounters a single unset instructions, which results in three possible
        // programs
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };

        searcher.searchSubTree(resumeFrom, true);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
        REQUIRE(tracker.getTotalSuccess() == 2);
        REQUIRE(tracker.getMaxStepsFound() == 1648533);
    }
    SECTION( "EscapeIntoSearch2" ) {
        // After 394 steps, does not encounter any new instructions until program escapes after
        // 3007566 steps.
        //
        // This then results in various programs: some that complete and others that hang (in two
        // types of detectable hang).
        //
        //   *       ?
        //   _ _ _ * ? ?
        // * _ o o o _ *
        // * * * * o _ ?
        // o _ _ o o *
        // _ * _ _ o *
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };

        searcher.searchSubTree(resumeFrom, true);

        REQUIRE(tracker.getTotalHangs(HangType::NO_DATA_LOOP) == 1);
        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 2);
        REQUIRE(tracker.getTotalSuccess() == 8);
        REQUIRE(tracker.getMaxStepsFound() == 3007571);
    }
}
