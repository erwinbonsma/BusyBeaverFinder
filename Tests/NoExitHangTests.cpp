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
    SECTION( "6x6-ForcedTurn" ) {
        // Program that contains a forced turn, i.e. a block that is entered with a zero value and
        // contains a delta instruction. From this it is clear that the exit will be a clockwise
        // turn. This fact needs to be exploited to detect that this program hangs.
        //
        // More specifically, the compiled program is shown below. Here it is clear that Block 8
        // can never be entered.
        //
        // 0 (0): INC 1 => 0/1, #Steps = 1
        // 1 (1): SHR 1 => 2/3, #Steps = 1
        // 2 (2): INC 1 => 4/5, #Steps = 2
        // 3 (3): -
        // 4 (38): -
        // 5 (39): SHR 1 => 6/7, #Steps = 1
        // 6 (40): INC 1 => 8/9, #Steps = 2
        // 7 (41): SHR 1 => 6/7, #Steps = 2
        // 8 (76): -
        // 9 (77): SHL 2 => 10/11, #Steps = 5
        // 10 (58): DEC 1 => 6/7, #Steps = 1
        // 11 (59): SHL 2 => 10/11, #Steps = 6
        //
        //     *
        //     _ _ _ *
        //   * o o _
        // * _ o * *
        // * o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-PreviouslyUndetected1" ) {
        // Hang that was not detected by the old NoExitHangDetector. It was not investigated why,
        // as it suffices that the new ExitFinder correclty detects it.
        //
        // *     *
        // o _ _ _ _ *
        // o   * o _
        // o   _ o *
        // o * _ o *
        // o     *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-PreviouslyUndetected2" ) {
        // Hang that was not detected by the old NoExitHangDetector. It was not investigated why,
        // as it suffices that the new ExitFinder correclty detects it.
        //
        //     *
        //     _ _ _ *
        //   * o o _
        // * _ o * *
        // * o *
        // o o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
    SECTION( "6x6-PreviouslyUndetected3" ) {
        // Hang that was not detected by the old NoExitHangDetector. It was not investigated why,
        // as it suffices that the new ExitFinder correclty detects it.
        //
        //       *
        //       _ _ *
        // *   * o _
        // o _ _ o *
        // _ * _ o *
        // _     *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalHangs(HangType::NO_EXIT) == 1);
    }
}
