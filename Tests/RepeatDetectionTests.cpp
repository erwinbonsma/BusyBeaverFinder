//
//  RepeatDetectionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 09/04/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include <iostream>

#include "catch.hpp"

#include "Utils.h"

TEST_CASE( "RepeatDetection", "[util][repeat]" ) {
    int buf[64];

    SECTION( "1234234" ) {
        int input[] = {1, 2, 3, 4, 2, 3, 4};
        int len = findRepeatedSequence(input, buf, 7);

        REQUIRE(len == 3);
    }
    SECTION( "1234123" ) {
        int input[] = {1, 2, 3, 4, 1, 2, 3};
        int len = findRepeatedSequence(input, buf, 7);

        REQUIRE(len == 0);
    }
    SECTION( "121314151415" ) {
        int input[] = {1, 2, 1, 3, 1, 4, 1, 5, 1, 4, 1, 5};
        int len = findRepeatedSequence(input, buf, 12);

        REQUIRE(len == 4);
    }
    SECTION( "12131413121" ) {
        int input[] = {1, 2, 1, 3, 1, 4, 1, 3, 1, 2, 1};
        int len = findRepeatedSequence(input, buf, 11);

        REQUIRE(len == 0);
    }
    SECTION( "11" ) {
        int input[] = {1, 1};
        int len = findRepeatedSequence(input, buf, 2);

        REQUIRE(len == 1);
    }
    SECTION( "1" ) {
        int input[] = {1};
        int len = findRepeatedSequence(input, buf, 1);

        REQUIRE(len == 0);
    }
}
