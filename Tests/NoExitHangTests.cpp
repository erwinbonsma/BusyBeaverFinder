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

bool canExitFromBlock(std::string programSpec, int numSteps) {
    Program program = Program::fromString(programSpec);
    auto interpretedProgram = std::make_shared<InterpretedProgramBuilder>();
    interpretedProgram->buildFromProgram(program);

    FastExecutor fastExecutor(1024);
    fastExecutor.setMaxSteps(numSteps);
    RunResult result = fastExecutor.execute(interpretedProgram);

    REQUIRE(result == RunResult::ASSUMED_HANG);

    ExitFinder exitFinder(program, *interpretedProgram);
    return exitFinder.canExitFrom(fastExecutor.lastProgramBlock());
}

TEST_CASE( "6x6 No Exit Hang Tests", "[hang][6x6][noexit]" ) {
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
        bool canExit = canExitFromBlock("Zr/0r1khGyGz7w", 19);

        REQUIRE(!canExit);
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
        bool canExit = canExitFromBlock("Zuq5UpVoW6L1bw", 58);

        REQUIRE(!canExit);
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
        bool canExit = canExitFromBlock("Zu65Etw4Rq20Gw", 26);

        REQUIRE(!canExit);
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
        bool canExit = canExitFromBlock("Zvv/AuU4a5v1vw", 19);

        REQUIRE(!canExit);
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
        bool canExit = canExitFromBlock("Zr70Ank3G2G37w", 47);

        REQUIRE(!canExit);
    }
    SECTION( "6x6-PreviouslyUndetected2" ) {
        // Hang that was not detected by the old NoExitHangDetector. It was not investigated why,
        // as it suffices that the new ExitFinder correclty detects it.
        //
        //       *
        //       _ _ *
        // *   * o _
        // o _ _ o *
        // _ * _ o *
        // _     *
        bool canExit = canExitFromBlock("Zv7/wrk0GyGz7w", 21);

        REQUIRE(!canExit);
    }
    SECTION( "6x6-IrregularSweep") {
        // An irregular sweep. Its turning point at one end of the sequence varies. The reason is
        // that it shifts DP two positions, which means it can ignore a mid-sequence zero.
        //
        // It has a meta-run loop with period 16: [12 14 5 16 12 14 5 18 12 14 5 20 12 14 5 22 ..]
        // Here 12 and 5 are the sweep loops. At the right of the sequence it uses four different
        // reversal sequences: 16, 18, 20, and 22.
        //
        // Finding this hang requires data-aware reachability analysis.
        //
        //     * * *
        //   * o o _ *
        //   o o o *
        //   _ o _
        // * _ _ o *
        // o o * *
        bool canExit = canExitFromBlock("Zvq+UtW8T4G1rw", 29);

        REQUIRE(!canExit);
    }
    SECTION( "6x6-IrregularSweep2") {
        // Another irregular sweep.
        //
        // It has a meta-meta-run loop.
        // The meta-run is as follows: #15[13 1 2 11 13 1 2 11] #22[12 1 2 11 12 1 2 11]
        // Here 1 and 11 are the sweep loops.
        //
        // Finding this hang requires data-aware reachability analysis.
        //
        //       *
        //   * * o _ *
        //   o o o *
        // * _ _ o *
        // * _   *
        // o o *
        bool canExit = canExitFromBlock("Zv7+ktW4G471vw", 36);

        REQUIRE(!canExit);
    }
    SECTION( "6x6-IrregularSweep3" ) {
        // An irregular sweep that was wrongly found by an early version of the Regular Sweep Hang
        // detector. Moving right, it shifts DP two positions each time. As a result, it skips over
        // some values. Initially, the sequence seems to have balanced grow, but it is broken up by
        // zeroes appearing mid-sequence, and the execution becomes quite chaotic.
        //
        // The meta-run is as follows:
        // #17[24 27 24 27]
        // #10(11 3)
        // #28[12 15 16 3 24 27 12 15 16 3 24 27 12 15 16 3]
        // #34(12 15 16 3 11 3 12 15 20 3)
        // #17[24 27 24 27]
        // #38(11 3 12 15 28 3 11 3)
        // #28[12 15 16 3 24 27 12 15 16 3 24 27 12 15 16 3]
        // #34(12 15 16 3 11 3 12 15 20 3)
        // Here all meta-run loops are of fixed size.
        //
        // It features three base-level loops of varying size. However, their size is not strictly
        // increasing over time:
        // #3[79 61 ..]
        // #15[25 43 ..]
        // #27[61 79 ..]
        //
        // Finding this hang requires data-aware reachability analysis.
        //
        //   *   * *
        // * o o o _ *
        //   * * o _
        //   - o o *
        // * _ _ o *
        // o _ o *
        bool canExit = canExitFromBlock("Zu65Uuk8W4G0bw", 419);

        REQUIRE(!canExit);
    }
}
