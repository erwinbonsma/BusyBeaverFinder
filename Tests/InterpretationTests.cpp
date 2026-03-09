//
//  InterpretationTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 08/05/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include <stdio.h>

#include "catch.hpp"

#include "ExhaustiveSearcher.h"

TEST_CASE( "7x7 Interpretation Tests", "[interpretation][7x7]" ) {
    SearchSettings settings {7};
    settings.maxSteps = 10000;
    ExhaustiveSearcher searcher {settings};

    auto tracker = std::make_unique<ProgressTracker>();
    searcher.attachProgressTracker(std::move(tracker));

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
        std::vector<Ins> resumeFrom = {
            Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN,
            Ins::DATA, Ins::DATA, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP,
            Ins::TURN, Ins::NOOP, Ins::DATA, Ins::DATA, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA,
            Ins::NOOP, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::NOOP, Ins::TURN, Ins::DATA,
            Ins::TURN, Ins::TURN, Ins::DATA, Ins::TURN, Ins::TURN, Ins::NOOP
        };
        searcher.findOne(resumeFrom);

        // TODO: Add actual test. For now, it suffices that the program does not throw an assertion
        // failure because there are too many entries.
        REQUIRE(true);
    }
}

TEST_CASE("Round-trip interpreted programs", "[program][blocks]") {
    std::string spec, steps;

    SECTION("RoundTrip-d++vWSlUsivlvlv1v8") {
        spec = "a+1-b b>1cd c+1-e dX e>1fg f+1-h g<1ze h>1ij i+1-k j<1yh k>1lm l<1no m<4qr n-1-p o<4qr p<3qr q+2-s r+1xs s>1tu t? u-1vw v>2lm w<1qr x? y-1-g zX";
        steps = "1 1 1 1 1 1 1 1 2 1 2 2 4 1 6 3 3 1 1 0 1 3 1 0 1 2";
    }
    SECTION("RoundTrip-Canonical-d++vWSlUsivlvlv1v8") {
        spec = "a+1-b b>1cd c+1-e dX e>1fg f+1-h g<1de h>1ij i+2-k j<1lh k>1mn l-1-g m<1on n<4pq o-1-r p+2-s q+1ts r<3pq s>1tu t? u-1vw v>2mn w<1pq";
        steps = "1 1 1 1 1 1 1 1 2 1 2 1 2 4 1 3 1 3 1 0 1 3 1";
    }
    SECTION("RoundTrip-d+7+UrkVpkPlblklu8") {
        spec = "a+1-b b>1cd c+1-e dX e>1fg f+1-h g<1Ae h>2ij i+1-k j-1wx k>1lm l<3no m<1BC n-1-p o+1qr p+2qr q-2-p r>1st s<2qr t-3uv u>1ij v<1zh w<1wx x+2yk y<2no z-1-g AX B-1-v C+1st";
        steps = "1 1 1 1 1 1 1 2 2 1 1 4 3 1 1 2 2 2 2 3 1 1 2 3 3 1 2 1 2";
    }
    SECTION("RoundTrip-Canonical-d+7+UrkVpkPlblklu8") {
        spec = "a+1-b b>1cd c+1-e dX e>1fg f+1-h g<1de h>2ij i+1-k j-1lm k>1no l<1lm m+2pk n<3qr o<1st p<2qr q-1-u r+1vw s-1-x t+1yz u+2vw v-2-u w>1yz x<1Ah y<2vw z-3Bx A-1-g B>1ij";
        steps = "1 1 1 1 1 1 1 2 2 1 1 2 3 4 3 3 1 1 1 2 2 2 2 1 2 3 1 1";
    }

    InterpretedProgramFromString program{spec, steps};
    std::string roundTripSpec = program.shortProgramString();
    std::string roundTripSteps = program.blockSizeString();
    REQUIRE(spec == roundTripSpec);
    REQUIRE(steps == roundTripSteps);
}
