//
//  ProgramTests.cpp
//  Tests
//
//  Created by Erwin on 23/09/2023.
//  Copyright © 2023 Erwin. All rights reserved.
//

#include <stdio.h>

#include "catch.hpp"

#include "Program.h"

TEST_CASE( "Program Encoding/Decoding Tests", "[program][encoding]" ) {
    SECTION( "2x2 All Turn" ) {
        std::string s = "Iqo";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == "****");
        REQUIRE(b64 == s);
    }
    SECTION( "2x2 All Data" ) {
        std::string s = "IlU";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == "oooo");
        REQUIRE(b64 == s);
    }
    SECTION( "3x3 All Turn" ) {
        std::string s = "M6qqg";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == std::string(9, '*'));
        REQUIRE(b64 == s);
    }
    SECTION( "3x3 All Data" ) {
        std::string s = "M1VVQ";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == std::string(9, 'o'));
        REQUIRE(b64 == s);
    }
    SECTION( "5x3 All Turn") {
        std::string s = "U6qqqqg";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == std::string(15, '*'));
        REQUIRE(b64 == s);
    }
    SECTION( "5x3 Data Border") {
        std::string s = "U6qgKqg";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == "******___******");
        REQUIRE(b64 == s);
    }
    SECTION( "4x4 Rainbow") {
        std::string s = "RBsbGxs";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == "_o*._o*._o*._o*.");
        REQUIRE(b64 == s);
    }
    SECTION( "5x5 All Turn") {
        std::string s = "Vaqqqqqqqo";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == std::string(25, '*'));
        REQUIRE(b64 == s);
    }
    SECTION( "5x5 Checkers") {
        std::string s = "VZmZmZmZmY";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == "*o*o*o*o*o*o*o*o*o*o*o*o*");
        REQUIRE(b64 == s);
    }
    SECTION( "7x7 All Turn") {
        std::string s = "d6qqqqqqqqqqqqqqqo";
        Program program = Program::fromString(s);

        std::string plain = program.toPlainString();
        std::string b64 = program.toString();
        REQUIRE(plain == std::string(49, '*'));
        REQUIRE(b64 == s);
    }
}
