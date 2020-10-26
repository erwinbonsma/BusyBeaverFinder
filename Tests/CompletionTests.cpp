//
//  CompletionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>

#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "6x6 Completion tests", "[success][6x6]" ) {
    ExhaustiveSearcher searcher(6, 6, 32);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 10000;
    settings.maxSteps = settings.maxHangDetectionSteps;
    settings.disableNoExitHangDetection = true;
    searcher.configure(settings);

    SECTION( "DivergingDeltaYetNoHang" ) {
        // A diverging change after 102 steps, however, not a hang as one value change touched zero
        // (it changed from one, to zero, back to one). At Step 102 this value is two (a diverging
        // delta) but it does not touch zero anymore, which means that subsequent program flow
        // diverges.
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
    SECTION( "IdenticalSnapshotDeltaButNonZeroNewlyVisited" ) {
        // An example where after 220 steps the snapshot delta is the same, but a newly visited
        // value was not zero. This hang was not detected, as there was an error in the check for
        // sequences extending to the left.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
    SECTION( "ComplexCountToNine" ) {
        // Program that was wrongly reported as hanging by an early version of the Periodic Hang
        // Detector refactored to use RunSummary.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
    SECTION( "PreviouslyAFalsePositiveOfExitFinder" ) {
        // Program that was wrongly reported as hanging by an early version of the Exit Finder with
        // reachability analysis. It failed due to a missing call to InterpretedProgram::enterBlock.
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
    SECTION( "FakeSweeper" ) {
        // Program that was wrongly reported as hanging by an earlier version of the Sweep Hang
        // detector
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getTotalSuccess() == 1);
    }
}

TEST_CASE( "7x7 One-Shot Completion tests", "[success][7x7][1-shot]" ) {
    ExhaustiveSearcher searcher(7, 7, 16384);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 1000000;
    settings.maxSteps = settings.maxHangDetectionSteps;
    settings.undoCapacity = settings.maxSteps;
    searcher.configure(settings);

    SECTION( "BB 7x7 #117273" ) {
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 117273);
    }
    SECTION( "BB 7x7 #140164" ) {
        //   *   * * *
        // * o . . o . *
        // o o * o o .
        // . * . o o *
        // .   . * o .
        // . * . . . . *
        // .       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::UNSET,
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 140164);
    }
    SECTION( "BB 7x7 #177557" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 177557);
    }
    SECTION( "BB 7x7 #422155" ) {
        //       *
        //   * _ o _ _ *
        // * _ o _ * _
        // o _ * o o _ *
        // _ o _ o o *
        // _ * _ o _ o *
        // _   * * * _
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 422155);
    }
    SECTION( "BB 7x7 #582562" ) {
        //   *   * * *
        // * o _ _ o _ *
        // o o * o o _
        // o * _ o o *
        // o   _ * o _
        // _ * _ _ _ _ *
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 582562);
    }
    SECTION( "BB 7x7 #690346" ) {
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 690346);
    }
    SECTION( "BB 7x7 #706369" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 706369);
    }
    SECTION( "BB 7x7 #847273" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 847273);
    }
    SECTION( "BB 7x7 #874581" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 874581);
    }
    SECTION( "BB 7x7 #950175" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 950175);
    }
    SECTION( "BB 7x7 #951921" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::UNSET
        };
        searcher.findOne(resumeFrom);

        REQUIRE(tracker.getMaxStepsFound() == 951921);
    }
}

void twoShotSearch(ExhaustiveSearcher& searcher, Ins* resumeStack, int numExpectedSteps) {
    ProgressTracker* tracker = searcher.getProgressTracker();

    searcher.findOne(resumeStack);

    if (tracker->getTotalLateEscapes() == 1) {
        assert(false); // Flawed test design? Somehow never triggered
        SearchSettings settings = searcher.getSettings();
        settings.maxHangDetectionSteps = 0; // Disable hang detection
        searcher.configure(settings);

        searcher.findOne(resumeStack);
    }

    REQUIRE(tracker->getMaxStepsFound() == numExpectedSteps);
}

TEST_CASE( "7x7 Two-Shot Completion tests", "[success][7x7][2-shot]" ) {
    ExhaustiveSearcher searcher(7, 7, 16384);
    ProgressTracker tracker(searcher);

    tracker.setDumpBestSofarLimit(INT_MAX);
    searcher.setProgressTracker(&tracker);

    SearchSettings settings = searcher.getSettings();
    settings.maxHangDetectionSteps = 100000;
    settings.undoCapacity = settings.maxHangDetectionSteps;
    settings.maxSteps = 10000000;
    searcher.configure(settings);

    SECTION( "BB 7x7 #1237792" ) {
        // Notable because it ends with DP on a high value: 31985.
        //
        //   *       *
        //   _ _ * * o *
        // * o o o o o *
        // * _ * o _ o
        // o _ _ o o *
        // _ *   * _
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 1237792);
    }
    SECTION( "BB 7x7 #1659389" ) {
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // * _   * _ _
        // o _ _ _ o *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP,
            Ins::DATA, Ins::TURN, Ins::TURN, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 1659389);
    }
    SECTION( "BB 7x7 #1842683" ) {
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::TURN, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 1842683);
    }
    SECTION( "BB 7x7 #3007569" ) {
        //   *       _
        //   _ _ _ * _
        // * _ o o o _ *
        // * * * * o _
        // o _ _ o o *
        // _ * _ _ o *
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 3007569);
    }
    SECTION( "BB 7x7 #8447143" ) {
        //   *       *
        //   _ _ * * _
        // * o o o o _ *
        // * _ * * o _ _
        // o _ _ o o *
        // _ * _ _ o *
        // _       *
        Ins resumeFrom[] = {
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 8447143);
    }
    SECTION( "BB 7x7 #9408043" ) {
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // o _ o * _ _
        // o _       *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN,
            Ins::NOOP, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::TURN, Ins::TURN, Ins::TURN, Ins::NOOP, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 9408043);
    }
    SECTION( "BB 7x7 #9607923" ) {
        //       * * *
        //     * o o _ *
        //   * o o o *
        //   _ o * o _
        // * _ o o o _ *
        // * _ * _ _ _ *
        // o _ _ o * *
        Ins resumeFrom[] = {
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::DATA, Ins::DATA,
            Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN,
            Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::NOOP, Ins::UNSET
        };

        twoShotSearch(searcher, resumeFrom, 9607923);
    }
}
