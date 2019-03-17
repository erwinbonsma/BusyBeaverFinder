//
//  NoExitHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "6x6 No Exit Hang Tests", "[hang][6x6][noexit]" ) {
    ExhaustiveSearcher searcher(6, 6, 256);
    ProgressTracker tracker(searcher);

    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxPeriodicHangDetectAttempts = 4;
    settings.initialHangSamplePeriod = 16;
    searcher.configure(settings);

    SECTION( "6x6-SweepWithBinaryCounter" ) {
        // Irregular sweep hang. At the left it extends normally, one cell per sweep. At the right
        // side it maintains a binary counter, and extends when it overflows.
        //
        // *
        // o _ * *
        // o o * o _ *
        // _ o _ o *
        // _ * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-ThreeWidthGlider" ) {
        // Generates an irregular glider that has a width of three data cells.
        //
        //   * * * *
        // * o o o _ *
        // * o o o o *
        // * _ o o *
        // * * _ *
        // o o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-Glider-PpOnlyReverses" ) {
        // Generates another glider. Here the program logic is intereseting. Once in the hang loop,
        // PP traverses a rectangular path. It never deviates from it. It only, occassionally,
        // briefly reverses.
        //
        //   *   * *
        // * o _ o _ *
        //   o   _ _
        // * _ o _ o *
        // * *   o *
        // o _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    // TODO: Find out why this was not yet detected by existing Regular Sweep Test
    SECTION( "6x6-UndetectedRegularSweep" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    // TODO: Find out why this was not yet detected by existing Regular Sweep Test
    SECTION( "6x6-UndetectedRegularSweep2" ) {
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-InnerSquare" ) {
        // Hang that was not detected by an earlier version of the detector. The cause was that
        // detection started at an instruction that was never visited again, while no new
        // instructions were visisted. As a result, the check if the loop could be exited was never
        // invoked, cause the check itself to hang.
        //
        //     *   *
        //   * o * _
        //   * _ _ o *
        //   o _ _ *
        // * _ * _
        // o o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs() == 1);
    }
}
