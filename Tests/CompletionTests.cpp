//
//  CompletionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 03/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"

TEST_CASE("6x6 Completion tests", "[success][6x6]") {
    HangExecutor hangExecutor(1024, 10000);
    hangExecutor.setMaxSteps(10000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("DivergingDeltaYetNoHang") {
        // A diverging change after 102 steps, however, not a hang as one value change touched zero
        // (it changed from one, to zero, back to one). At Step 102 this value is two (a diverging
        // delta) but it does not touch zero anymore, which means that subsequent program flow
        // diverges.
        RunResult result = hangExecutor.execute("ZgiIFkJEVmQgiA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 116);
    }
    SECTION("IdenticalSnapshotDeltaButNonZeroNewlyVisited") {
        // An example where after 220 steps the snapshot delta is the same, but a newly visited
        // value was not zero. This hang was not detected, as there was an error in the check for
        // sequences extending to the left.
        RunResult result = hangExecutor.execute("ZgqCUhWIUIBlqA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 373);
    }
    SECTION("ComplexCountToNine") {
        // Program that was wrongly reported as hanging by an early version of the Periodic Hang
        // Detector refactored to use RunSummary.
        RunResult result = hangExecutor.execute("ZiiJUoBEZGBEiA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 148);
    }
    SECTION("PreviouslyAFalsePositiveOfExitFinder") {
        // Program that was wrongly reported as hanging by an early version of the Exit Finder with
        // reachability analysis. It failed due to a missing call to InterpretedProgram::enterBlock.
        RunResult result = hangExecutor.execute("ZiABCAFoUoJFiA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 24);
    }
    SECTION("FakeSweeper") {
        // Program that was wrongly reported as hanging by an earlier version of the Sweep Hang
        // detector
        RunResult result = hangExecutor.execute("ZgqCFiAAGJWFoA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 171);
    }
}

TEST_CASE("7x7 One-Shot Completion tests", "[success][7x7]") {
    HangExecutor hangExecutor(16384, 1000000);
    hangExecutor.setMaxSteps(40000000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("BB 7x7 #117273") {
        RunResult result = hangExecutor.execute("dwoAlShaIhJBYIGAKA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 117273);
    }
    SECTION("BB 7x7 #140164") {
        //   *   * * *
        // * o . . o . *
        // o o * o o .
        // . * . o o *
        // .   . * o .
        // . * . . . . *
        // .       *
        RunResult result = hangExecutor.execute("dyKiQSWUCFgCQIAgCA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 140164);
    }
    SECTION("BB 7x7 #177557") {
        RunResult result = hangExecutor.execute("dwogkSlYBgQJWgSFoA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 177557);
    }
    SECTION("BB 7x7 #422155") {
        // This caries out a sweep but one that eventually terminates. Properties:
        // - Rightwards sweep shifts one each iteration and does not change the sequence.
        //   It ends on zero, changing it to 1, thereby extending the sequence (steady growth).
        // - Leftwards sweep shifts two each iteration. It increments half of the values in the
        //   sequence by one (non-uniform change). It has two possible transitions:
        //   - It ends on -1, leaving this value unchanged, but decreasing its neighbour (inside
        //     the sequence by three).
        //   - It ends on zero, converting it to -2, thereby extending the sequence.
        //
        // The program terminates when the left-sweep encounters a -1 value that neighbours a 3.
        // When this happens is hard to foresee, as there are multiple simple interactions that
        // together produce a complex behavior:
        // - The two-cell shift means that not always the same -1 value is encountered.
        // - A fixed -1 exit can be changed into a non-exit once its left neighbour is also a -1.
        // - This occassionally means that the left sweep does not encounter a -1 exit, but
        //   reaches the end of the sequence. This results in a -2, which later can change into a
        //   -1 exit.
        //
        //       *
        //   * _ o _ _ *
        // * _ o _ * _
        // o _ * o o _ *
        // _ o _ o o *
        // _ * _ o _ o *
        // _   * * * _
        RunResult result = hangExecutor.execute("dwIAhChIElIRYIRgqA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 422155);
    }
    SECTION("BB 7x7 #582562") {
        //   *   * * *
        // * o _ _ o _ *
        // o o * o o _
        // o * _ o o *
        // o   _ * o _
        // _ * _ _ _ _ *
        // _       *
        RunResult result = hangExecutor.execute("dyKiQSWUGFhCQIAgCA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 582562);
    }
    SECTION("BB 7x7 #690346") {
        RunResult result = hangExecutor.execute("dyICUgGUpVhGAIWAiA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 690346);
    }
    SECTION("BB 7x7 #706369") {
        RunResult result = hangExecutor.execute("dyiiUCJFgZiFYmAFiA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 706369);
    }
    SECTION("BB 7x7 #847273") {
        RunResult result = hangExecutor.execute("dwgAkIJQAaCFWigEBg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 847273);
    }
    SECTION("BB 7x7 #874581") {
        RunResult result = hangExecutor.execute("dwoglSlYIWQZWgSFoA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 874581);
    }
    SECTION("BB 7x7 #950175") {
        RunResult result = hangExecutor.execute("dygiUiBhiVaBSiRkFg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 950175);
    }
    SECTION("BB 7x7 #951921") {
        RunResult result = hangExecutor.execute("dyiiQSJUAZiFYmEliA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 951921);
    }
    SECTION("BB 7x7 #1237792") {
        // Notable because it ends with DP on a high value: 31985.
        //
        //   *       *
        //   _ _ * * o *
        // * o o o o o *
        // * _ * o _ o
        // o _ _ o o *
        // _ *   * _
        // _       *
        RunResult result = hangExecutor.execute("dyAgCmlVokRBYIgACA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 1237792);
    }
    SECTION("BB 7x7 #1659389") {
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // * _   * _ _
        // o _ _ _ o *
        RunResult result = hangExecutor.execute("dyKCRIKUgWCBWggEBg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 1659389);
    }
    SECTION("BB 7x7 #1842683") {
        RunResult result = hangExecutor.execute("dwgAkIJQgaSFWmgFgg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 1842683);
    }
    SECTION("BB 7x7 #3007569") {
        //   *       _
        //   _ _ _ * _
        // * _ o o o _ *
        // * * * * o _
        // o _ _ o o *
        // _ * _ _ o *
        // _       *
        RunResult result = hangExecutor.execute("dyAAAghUqpBBYIGACA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 3007569);
    }
    SECTION("BB 7x7 #8447143") {
        //   *       *
        //   _ _ * * _
        // * o o o o _ *
        // * _ * * o _ _
        // o _ _ o o *
        // _ * _ _ o *
        // _       *
        RunResult result = hangExecutor.execute("dyAgCglUopBBYIGACA");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 8447143);
    }
    SECTION("BB 7x7 #9408043") {
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // o _ o * _ _
        // o _       *
        RunResult result = hangExecutor.execute("dyKCRIKUgWCBWRgEAg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 9408043);
    }
    SECTION("BB 7x7 #9607923") {
        //       * * *
        //     * o o _ *
        //   * o o o *
        //   _ o * o _
        // * _ o o o _ *
        // * _ * _ _ _ *
        // o _ _ o * *
        RunResult result = hangExecutor.execute("dwKgJSJWAZCFSiAkGg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 9607923);
    }
    SECTION("BB 7x7 #10981971") {
        // Very similar to BB 22.6M. The main logic is identical. Only the bootstrap differs.
        //
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // * _   * _ _
        // o _ _ o o *
        RunResult result = hangExecutor.execute("dyKCRIKUgWCBWggEFg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 10981971);
    }
    SECTION("BB 7x7 #22606881") {
        //   *   * *
        // * o _ o _ *
        //   * * o o _ *
        //   _ o o * _
        // * _ _ o o o *
        // o o o * _ _
        // _ _       *
        RunResult result = hangExecutor.execute("dyKCRIKUgWCBWVgAAg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 22606881);
    }
    SECTION("BB 7x7 #33207907") {
        //   * *   *
        // * o o _ _ *
        //   * o o _ _ *
        //   _ o * * _
        // * _ o o o o *
        // * _ * _ _ o *
        // o _ _ _ o *
        RunResult result = hangExecutor.execute("dyiCUIJQgaCFWiBkBg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 33207907);
    }
}
