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
    hangExecutor.setMaxSteps(60000000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-OldFalsePositive3") {
        // Program exhibits a behavior that resembles a simple hang. Steady growth at its right,
        // and a fixed point at its left. However, this fixed point used to be classified
        // incorrectly. The transition sequence (plus subsequent bootstrap of the outgoing loop)
        // actually moves one cell towards zero.
        //
        //   *     * *
        //   _ _ * o _ *
        // * o _ o o o *
        // * * * o * o
        // o o o o * o
        // _ * _ o _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dyCgCSkVqmRVkIRAIg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 348);
    }
    SECTION("7x7-OldFalsePositive4") {
        // Program exhibits a behavior that resembles a simple hang. Steady growth at both sides.
        // However, there is an isolated one at the left of the sequence that breaks the leftward
        // growth and causes the program to terminate.
        //
        //       *
        //     * o _ *
        //     * o * _ *
        // *   _ o * o
        // o o o o o o *
        // _ * _ _ _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dwIAJICYoGRVWIBAIg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 257);
    }
    SECTION("7x7-OldFalsePositive5") {
        // Very similar in behavior to the previous program.
        //
        //   *   * *
        // * o o _ _ *
        //     * o o *
        // *   _ o * _ *
        // o o o o o o *
        // _ * _ o o o *
        // _   * *   *
        RunResult result = hangExecutor.execute("dyKCUICWIGJVWIVgog");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 342);
    }
    SECTION("7x7-OldFalsePositive6") {
        // Very similar in behavior to the previous program. Possibly a bit more subtle.
        //
        //       *
        //     * o _ _ *
        //     * o   *
        // *   _ o * _
        // o o o o o o *
        // _ * _ _ _ o
        // _     *   *
        RunResult result = hangExecutor.execute("dwIAJCCSIGBVWIBAIg");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 317);
    }
    SECTION("7x7-OldFalsePositive7") {
        // Was previously reported as a sweep hang as it failed to detect that the sweep hang
        // would not continue forever.
        //
        // *       * *
        // o _ _ * o _ *
        // _   o _ o o *
        // _   * _ _ o
        // _ _ o _ o *
        // _ * o o _ *
        // _   *   *
        RunResult result = hangExecutor.execute("d7+tCSNFjgcEbJSzu8");

        REQUIRE(result == RunResult::SUCCESS);
    }
    SECTION("7x7-OldFalsePositive8") {
        // Was previously reported as a sweep hang as it failed to detect that the sweep hang
        // would not continue forever.
        //
        //         * *
        // *   * * _ _ *
        // o o o _ o _ *
        // _ * o o o _ *
        // _   *   o *
        // _ * _ _ o *
        // _       *
        RunResult result = hangExecutor.execute("dz+uKCVEiVI4bIGzu8");

        REQUIRE(result == RunResult::SUCCESS);
    }
    SECTION("7x7-OldFalsePositive9") {
        // Enters various sweeps that each terminate, but then transition into another sweeps.
        // Eventually a state is reached where by sweep termination terminates the program.
        //
        //       *   *
        // *   * o o _ *
        // o o _ o * _
        // o   * o o o *
        // _   _ o o *
        // _ * _ _ o
        // _     * *
        RunResult result = hangExecutor.execute("dz4uJSUYHlYxbIEDK8");

        REQUIRE(result == RunResult::SUCCESS);
    }
    SECTION("7x7-OldFalsePositive10") {
        // Terminates after 9431 steps
        //
        //   *   *   *
        // * o * o o _ *
        // o o * o * _
        // o _ * o o o *
        // _ _ _ o o *
        // _ * _ _ o
        // _     * *

        RunResult result = hangExecutor.execute("d+7uZSWY0lYBbIED68");

        REQUIRE(result == RunResult::SUCCESS);
    }

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
    SECTION("BB 7x7 #10268897") {
        //   * *   * *
        // * o _ _ o _ *
        //   * o o o o *
        //   o o * o *
        // * _ o o o *
        // * * * _ o _ *
        // o _ _ o *
        RunResult result = hangExecutor.execute("d+uuQS5VtZuFbqEkG8");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 10268897);
    }
    SECTION("BB 7x7 #19904958") {
        // ? * ? ? * * ?
        // ? _ _ * o _ *
        // * o o o o _ *
        // * * * ? _ * ?
        // o o o o o * ?
        // o * _ _ o o *
        // _ ? * ? * * ?
        RunResult result = hangExecutor.execute("d++vCSlUqstVbYFjus");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 19904958);
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
    SECTION("BB 7x7 #23822389") {
        // Notable because it uses relatively few (19) data cells. It exhibits a behavior that is
        // partially a glider and partially a sweep. The number of non-zero data values varies
        // from three to five (at most?). With three values it executes a glider loop, where the
        // left value is the loop counter that is decreased by one each iteration. The other two
        // counters are respectively increased by three and decreased by one. The middle counter
        // starts negative and becomes positive. Whether the glider deviates from its three-counter
        // mode depends on how this middle value crosses the zero (which depends on the modulus
        // of its initial value). Ocassionally this results in an extra non-zero value.
        //
        //   * *   *
        // * o _ _ _ _ *
        //   _ o * _ *
        //   * _ o o o *
        // * o o o _ o *
        // * * * o _ o
        // o _ _ _ o *
        RunResult result = hangExecutor.execute("d+u+QCxi+FaVGqR0Bs");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 23822389);
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
    SECTION("BB 7x7 #59924237") {
        //     * *   o
        //   * o o o _ *
        // * _ o o * *
        // o _ * o o o *
        // _ o _ o o *
        // _ * _ _ o *
        // _     * *
        RunResult result = hangExecutor.execute("d/rflSha0lYRbIGz68");

        REQUIRE(result == RunResult::SUCCESS);
        REQUIRE(hangExecutor.numSteps() == 59924237);
    }
    SECTION("7x7-OldFalsePositive1") {
        // Program exhibits a behavior that resembles an irregular sweep hang, with an aperiodic
        // appendix at its right. However, the leftwards sweep moves DP by two cells each
        // iteration. All leftward sweeps, however, seem to end on the first zero at the left. This
        // is caused by how the "bits" in the binary appendix at the right toggle. However, after
        // 2000 steps the left sweep lands on the second zero, which causes the program to exit.
        //
        // *     * *
        // o _ * _ _ _ *
        // _ _ * o o _
        // _ _ o o o *
        // _ o _ o _ *
        // _ * _ o o *
        // _   * * *
        RunResult result = hangExecutor.execute("d4KBICCUAVgRIIWAqA");

        REQUIRE(result == RunResult::SUCCESS);
    }
    SECTION("7x7-OldFalsePositive2") {
        // Program exhibits a behavior that resembles an irregular sweep hang, with an aperiodic
        // appendix at its right. However, the sequence that changes a -1 bit in the binary
        // appendix to a -2 requires that the cell located two cells to its left has a non-zero
        // value. This is not the case for a -1 value near the end of the appendix.
        //
        //     *     *
        //   * _ _ * o *
        // * _ o o o _ *
        // *   _ * o _
        // o o o * o *
        // _   * _ o *
        // _       *
        RunResult result = hangExecutor.execute("dwgggmhUoJBWYCGACA");

        REQUIRE(result == RunResult::SUCCESS);
    }
}
