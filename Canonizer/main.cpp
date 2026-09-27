//
//  Canonizer.cpp
//  Canonizer
//
//  Created by Erwin on 03/03/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include <string>
#include <iostream>

#include "cxxopts.hpp"

#include "Program.h"
#include "InterpretedProgramBuilder.h"
#include "InterpretedProgramCanonizer.h"


void outputInterpretedProgram(InterpretedProgram& program) {
}

void canonizeProgram(std::string& programSpec, bool verbose = false) {
    Program program = Program::fromString(programSpec);

    InterpretedProgramBuilder builder;
    builder.buildFromProgram(program);

    auto canonizer = InterpretedProgramCanonizer::canonizeProgram(builder);

    std::cout << programSpec;
    std::cout << "\t" << canonizer.shortProgramString();
    std::cout << "\t" << canonizer.blockSizeString() << std::endl;

    if (verbose) {
        builder.dump();
        canonizer.dump();
    }
}

int main(int argc, char * argv[]) {
    cxxopts::Options options("BB-Canonizer", "Canonizer for 2L-BB Programs");
    options.add_options()
        ("program", "Program specification", cxxopts::value<std::string>())
        ("help", "Show help");
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    std::string programSpec;
    if (args.count("program")) {
        programSpec = args["program"].as<std::string>();
        canonizeProgram(programSpec, true);
    } else {
        while (std::getline(std::cin, programSpec)) {
            canonizeProgram(programSpec);
        }
    }

    return 0;
}
