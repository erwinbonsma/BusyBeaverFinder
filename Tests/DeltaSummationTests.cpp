//
//  DeltaSummationTests.cpp
//  Tests
//
//  Created by Erwin on 03/01/2021.
//  Copyright © 2021 Erwin. All rights reserved.
//

#include <iostream>

#include "catch.hpp"

#include "Utils.h"

TEST_CASE( "Delta Summation", "[util][delta-sum]" ) {
    SECTION( "1 ===> 6" ) {
        std::set<int> deltas = { 1 };
        bool possible = deltasCanSumTo(deltas, 6);

        REQUIRE(possible);
    }
    SECTION( "1 =X=> -6" ) {
        std::set<int> deltas = { 1 };
        bool possible = deltasCanSumTo(deltas, -6);

        REQUIRE(!possible);
    }
    SECTION( "2 ===> 6" ) {
        std::set<int> deltas = { 2 };
        bool possible = deltasCanSumTo(deltas, 6);

        REQUIRE(possible);
    }
    SECTION( "-2 =X=> 6" ) {
        std::set<int> deltas = { -2 };
        bool possible = deltasCanSumTo(deltas, 6);

        REQUIRE(!possible);
    }
    SECTION( "2 =X=> 5" ) {
        std::set<int> deltas = { 2 };
        bool possible = deltasCanSumTo(deltas, 5);

        REQUIRE(!possible);
    }
    SECTION( "2, 3 ===> 7" ) {
        std::set<int> deltas = { 2, 3 };
        bool possible = deltasCanSumTo(deltas, 7);

        REQUIRE(possible);
    }
    SECTION( "2, 3 =X=> 1" ) {
        std::set<int> deltas = { 2, 3 };
        bool possible = deltasCanSumTo(deltas, 1);

        REQUIRE(!possible);
    }
    SECTION( "2, -3 ===> 1" ) {
        std::set<int> deltas = { 2, -3 };
        bool possible = deltasCanSumTo(deltas, 1);

        REQUIRE(possible);
    }
    SECTION( "2, -3 ===> 7" ) {
        std::set<int> deltas = { 2, -3 };
        bool possible = deltasCanSumTo(deltas, 7);

        REQUIRE(possible);
    }
    SECTION( "2, -2 =X=> 1" ) {
        std::set<int> deltas = { 2, -2 };
        bool possible = deltasCanSumTo(deltas, 1);

        REQUIRE(!possible);
    }
    SECTION( "2, -2 ===> 8" ) {
        std::set<int> deltas = { 2, -2 };
        bool possible = deltasCanSumTo(deltas, 8);

        REQUIRE(possible);
    }
    SECTION( "3, 4 =X=> 5" ) {
        std::set<int> deltas = { 3, 4 };
        bool possible = deltasCanSumTo(deltas, 5);

        REQUIRE(!possible);
    }
    SECTION( "3, 4 ===> 7" ) {
        std::set<int> deltas = { 3, 4 };
        bool possible = deltasCanSumTo(deltas, 7);

        REQUIRE(possible);
    }
    SECTION( "3, 4 ===> 13" ) {
        std::set<int> deltas = { 3, 4 };
        bool possible = deltasCanSumTo(deltas, 13);

        REQUIRE(possible);
    }
    SECTION( "8, -7 ===> 5" ) {
        std::set<int> deltas = { 8, -7 };
        bool possible = deltasCanSumTo(deltas, 5);

        REQUIRE(possible);
    }
}
