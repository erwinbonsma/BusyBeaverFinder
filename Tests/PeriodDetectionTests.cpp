//
//  PeriodDetectionTests.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 06/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include <stdio.h>
#include <iostream>

#include "catch.hpp"

#include "Utils.h"

bool compareArrays(int* array1, int* array2, int len) {
    for (int i = 0; i < len; i++) {
        if (array1[i] != array2[i]) {
            return false;
        }
    }
    return true;
}

TEST_CASE( "Z-array", "[util]" ) {
    int output[64];

    SECTION( "abcabcabcabc" ) {
        const char* s = "abcabcabcabc";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 0, 0, 9, 0, 0, 6, 0, 0, 3, 0, 0 };

        REQUIRE(compareArrays(output, expected, len));
    }
    SECTION( "abcabcabcab" ) {
        const char* s = "abcabcabcab";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 0, 0, 8, 0, 0, 5, 0, 0, 2, 0 };

        REQUIRE(compareArrays(output, expected, len));
    }
    SECTION( "abcabcdabcab" ) {
        const char* s = "abcabcdabcab";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 0, 0, 3, 0, 0, 0, 5, 0, 0, 2, 0 };

        REQUIRE(compareArrays(output, expected, len));
    }
    SECTION( "abcabcabcxy" ) {
        const char* s = "abcabcabcxy";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 0, 0, 6, 0, 0, 3, 0, 0, 0, 0 };

        REQUIRE(compareArrays(output, expected, len));
    }
    SECTION( "aaaaa" ) {
        const char* s = "aaaaa";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 4, 3, 2, 1 };

        REQUIRE(compareArrays(output, expected, len));
    }
    SECTION( "abab_ababab_abab" ) {
        const char* s = "abab_ababab_abab";
        int len = (int)strlen(s);

        calculateZArray(s, output, len);

        int expected[] = { 0, 0, 2, 0, 0, 4, 0, 9, 0, 2, 0, 0, 4, 0, 2, 0 };

        REQUIRE(compareArrays(output, expected, len));
    }
}

TEST_CASE( "findPeriod", "[util]" ) {
    int output[64];

    SECTION( "abcabcabcabc" ) {
        const char* s = "abcabcabcabc";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 3);
    }
    SECTION( "abcabcabcab" ) {
        const char* s = "abcabcabcab";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 3);
    }
    SECTION( "abcabcdabcab" ) {
        const char* s = "abcabcdabcab";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 7);
    }
    SECTION( "abcabcabcxy" ) {
        const char* s = "abcabcabcxy";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == len);
    }
    SECTION( "aaaaa" ) {
        const char* s = "aaaaa";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 1);
    }
    SECTION( "ababcabababcabab" ) {
        const char* s = "ababcabababcabab";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 7);
    }
    SECTION( "abab_ababab_abab" ) {
        const char* s = "abab_ababab_abab";
        int len = (int)strlen(s);

        int period = findPeriod(s, output, len);

        REQUIRE(period == 7);
    }
}
