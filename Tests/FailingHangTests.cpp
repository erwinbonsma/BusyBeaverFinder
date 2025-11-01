//
//  FailingHangTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 17/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "catch.hpp"

#include "HangExecutor.h"

TEST_CASE("6x6 Failing Hang tests", "[hang][regular][sweep][6x6][fail]") {
    HangExecutor hangExecutor(1024, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();
}

TEST_CASE("6x6 Failing Irregular Sweep Hang tests", "[hang][sweep][irregular][6x6][fail]") {
    HangExecutor hangExecutor(1024, 20000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

    // No known failures yet
}

TEST_CASE("6x6 Failing Irregular Other Hangs", "[hang][irregular][6x6][fail]") {
    HangExecutor hangExecutor(2048, 20000);
    hangExecutor.setMaxSteps(1000000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("6x6-IrregularHopScotch") {
        // This program executes a curious sweep. The sweep does not have a sweep body. At the left
        // there's a fixed, ever-increasing exit. Directly attached is an a-periodically growing
        // appendix which consists of three values: 0, -1 and -2.
        //
        // The right-moving loop moves DP two steps. It increases the values it lands on by one,
        // converting -2's to -1's. It exits on zero.
        //
        // The left-moving sweep is more chaotic. It clears -1 values and leaves -2 values behind.
        // It only converts half of the -1 values to -2. The others it resets to zero.
        //
        //       * *
        //   * * o _ *
        // * o o o _ *
        // * _ _ _ _ *
        // * * o o *
        // o _ o *
        //
        // Analysis is complex as the run summary cannot be collapsed into a higher-level loop:
        // #54
        // #39*1.0 #40 #124*0.4      #64*2.1 #40 #124*1.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*6.0 #40 #124*3.0 #138 #64*2.1 #40 #124*1.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*3.0 #40 #124*1.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*4.1 #40 #124*2.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*3.0 #40 #124*1.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*3.1 #40 #124*1.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*2.1 #40 #124*1.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*5.0 #40 #124*2.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*3.1 #40 #124*1.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*2.1 #40 #124*1.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*4.0 #40 #124*2.0 #138 #64*2.1 #40 #124*1.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*3.0 #40 #124*1.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        // #54
        // #39*1.0 #40 #124*0.4      #64*6.1 #40 #124*3.3                      #139
        // #39*1.0 #40 #124*0.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*3.0 #40 #124*1.4      #64*1.1 #40 #124*0.6 #64*0.1 #40 #124*0.3 #139
        // #39*2.0 #40 #124*0.4
        // #48*2.0
        RunResult result = hangExecutor.execute("Zv6+kpUoAqW0bw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-DualEndedIrregularSweep") {
        // Sweep hang with an irregularly growing appendix at both ends.
        //
        // On top of that, the behavior at the right side is unusual. Here, the in-sweep exit is
        // zero and the toggle value is one. It toggles values in the appendix when the sweep
        // returns, but only (about) half of the appendix values it traverses. It sets the
        // immediate neighbour to zero, and every second value in the appendix.
        //
        // The appendix at the left has an in-sweep exit of -1, and toggle value of -2. The sweep
        // body consists of two values that both move towards negative infinity.
        //
        // The data after 1000 steps is: ... 0 -2 -2 -2 -1 -4 -13 1 1 1 1 1 1 0 ...
        //
        //     * * *
        //   * o o _ *
        //   * o o o *
        // * _ o o *
        // * o * *
        // o o *
        //
        // Analysis is complex as the run summary cannot be collapsed into a higher-level loop:
        // #97*2.0 #59      #41*2.0 #43 #103*1.0 #104
        // #97*3.0 #59      #41*3.0 #43 #103*2.2
        //         #59      #41*6.0 #43          #57*2.4 #89
        //                  #41*2.0 #43          #57*0.5
        // #97*3.3     #106 #41*4.0 #43 #103*3.2
        //         #59      #41*6.0 #43          #57*2.4 #89
        //                  #41*2.0 #43          #57*0.4 #89
        //                  #41*3.0 #43          #57*0.5
        // #97*5.0 #59      #41*5.0 #43          #57*0.5
        // #97*3.0 #59      #41*3.0 #43 #103*2.2
        //         #59      #41*8.0 #43          #57*3.4 #89
        //                  #41*2.0 #43          #57*0.5
        // #97*4.3     #106 #41*5.0 #43 #103*4.2
        //         #59      #41*7.0 #43          #57*2.4     #108
        //                  #41*3.0 #43 #103*2.2
        //         #59      #41*4.0 #43 #103*1.0                  #104
        // #97*2.0 #59      #41*2.0 #43 #103*1.0                  #104
        // #97*5.0 #59      #41*5.0 #43 #103*4.2
        //         #59      #41*9.0 #43          #57*3.4     #108
        //                  #41*3.0 #43 #103*2.2
        //         #59      #41*4.0 #43 #103*1.0                  #104
        // #97*2.0 #59      #41*2.0 #43 #103*1.0                  #104
        // #97*5.3     #106 #41*6.0 #43 #103*5.2
        //         #59      #41*9.0 #43          #57*2.4
        //                              #103*3.2
        //         #59      #41*5.0 #43          #57*0.5
        RunResult result = hangExecutor.execute("Zvq+UuVoW5r1vw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-BodylessIrregularSweep") {
        // This used to be detected, but not anymore since commit f0054bfc.
        //     * *
        //   * o o _ *
        //   _ _ o *
        // * _ _ o *
        // * o * *
        // o o *
        //
        // It only ends up in a loop in the meta-meta run summary.
        //
        // Run summary:
        // #30 #34*3.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*3.0 #20
        //     #34*2.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*4.0 #20
        //     #34*3.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*3.0 #20
        //     #34*2.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*4.1
        // #30 #34*4.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*3.0 #20
        //     #34*2.0 #40 #18*2.0 #20
        //     ...
        //     #34*2.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*5.1
        // #30 #34*5.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*3.0 #20
        //     ...
        //     #34*2.0 #40 #18*2.0 #20
        //     #34*1.0 #40 #18*6.1
        // #30 #34*6.0 #40 #18*2.0 #20
        //     ...
        //
        // Meta-run summary: ... #18 #26*7.3 #18 #26*15.3 #18 #26*31.3 #18 #26*63.3 #18 #26*127.3
        // with:
        // 18 = 30
        // 26 = 34 40 18 20
        //
        // Meta-meta-run summary: ... #6*6.0
        // with: 6 = 18 26
        RunResult result = hangExecutor.execute("Zvr+UsG4G5r1vw");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-IrregularSweepWithAlternatingEndSweepTransition") {
        // The sweep hang has an irregular end at its right, consisting of zeroes and ones. The
        // rightward sweep ends on the first zero.
        //
        //   *   * *
        // * o o _ _ *
        // * o * o o *
        // o o * o *
        // o * _ o *
        // o     *
        //
        // It gets stuck in the following meta-run loop: 28 29 13 15 28 29 13 15 28 35
        //
        // The run-summary is as follows:
        // ...
        // #28*3.1  #29 #13*5.0  #15 #28*5.1  #29 #13*7.0  #15 #28*1.0 #35*1.0
        // #28*5.1  #29 #13*7.0  #15 #28*7.1  #29 #13*10.0 #15 #28*1.0 #35*2.0
        // #28*7.1  #29 #13*9.0  #15 #28*9.1  #29 #13*11.0 #15 #28*1.0 #35*1.0
        // #28*9.1  #29 #13*11.0 #15 #28*11.1 #29 #13*16.0 #15 #28*1.0 #35*4.0
        // ...
        //
        // There are two possible end-sweep transitions at the irregular end, and the hang
        // alternates between both. The first is a plain one: #15. The second is one that
        // contains a loop whose length varies depending on the amount of ones in the irregular
        // end: #15 #28*1.0 #35*n
        //
        // There is a mid-sweep transition, but only for half of the leftward sweeps.
        RunResult result = hangExecutor.execute("Zu65Qpllm2G37w");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
    SECTION("6x6-IrregularSweepWithZeroesInAppendix") {
        // A truly binary counter. It actually uses ones and zeros, and also properly generates
        // binary numbers (only with most-significant bit at the right).
        //
        // It is similar in behavior to 6x6-IrregularSweepWithAlternatingEndSweepTransition
        //
        // The meta-run summary ends up in this loop:
        //   22 23 10 12 22 23 10 12 22 27
        //
        // Where the run summary looks as follows:
        //   #22*44.1 #23 #10*46.0 #12 #22*46.1 #23 #10*51.0 #12 #22*1.0 #27*4.0
        //   #22*46.1 #23 #10*48.0 #12 #22*48.1 #23 #10*50.0 #12 #22*1.0 #27*1.0
        //   #22*48.1 #23 #10*50.0 #12 #22*50.1 #23 #10*53.0 #12 #22*1.0 #27*2.0
        //   #22*50.1 #23 #10*52.0 #12 #22*52.1 #23 #10*54.0 #12 #22*1.0 #27*1.0
        //
        // Here the 27 loop is traversing the binary counter, flipping ones to zeros. After
        // having done so, it switches to the 22 loop for the leftward traversal of the sweep
        // body. The one-iteration 22 loop preceding the 27-loop murks the waters. To facilitate
        // analysis, it should be considered part of transition sequence 12.
        //
        //   *   * *
        // * o _ _ _ *
        // o o * o o *
        // o   * o *
        // _ * _ o *
        // _     *
        RunResult result = hangExecutor.execute("ZiKJAllkmCGAIA");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}

TEST_CASE("7x7 undetected hangs", "[hang][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    SECTION("7x7-UndetectedHang3") {
        // Program that causes Periodic Sweep Hang Detector to create transition-groups with
        // unique transitions. This caused an assertion to fail, which after analysis, has been
        // removed.
        //
        // *     * *
        // o _ _ _ _ _ *
        // _   * _ o _
        // _ * o o o *
        // _   _ o _
        // _ * _ _ _ o *
        // _   * * * *
        RunResult result = hangExecutor.execute("d4KBACCECVgBAIBgqg");

        // TEMP: Should not yet be detected with current logic. Eventually it should be detected.
        REQUIRE(result == RunResult::ASSUMED_HANG);
    }
}

TEST_CASE("7x7 false positives", "[success][7x7][fail]") {
    HangExecutor hangExecutor(16384, 100000);
    hangExecutor.setMaxSteps(100000);
    hangExecutor.addDefaultHangDetectors();

    // No known failures yet
}
