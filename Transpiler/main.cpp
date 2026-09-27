//
//  Transpiler.cpp
//  Transpiler
//
//  Created by Erwin on 27/09/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//


#include <string>
#include <iostream>

#include "cxxopts.hpp"

#include "Program.h"
#include "InterpretedProgramBuilder.h"
#include "InterpretedProgramCanonizer.h"


void outputInterpretedProgram(InterpretedProgram& program) {
    std::cout << "\t" << program.shortProgramString();
    std::cout << "\t" << program.blockSizeString() << std::endl;
}

int main(int argc, char * argv[]) {
    cxxopts::Options options("BB-Transpiler", "Transpiler for 2L-BB Programs");
    options.add_options()
        ("l,loopunroll", "Loop unroll size", cxxopts::value<int>())
        ("d,datasize", "Data size", cxxopts::value<int>())
        ("max-steps", "Maximum program execution steps", cxxopts::value<int>())
        ("program", "Program specification (interpreted)", cxxopts::value<std::string>())
        ("block-sizes", "Size specificatino of each program block", cxxopts::value<std::string>())
        ("help", "Show help");
    auto args = options.parse(argc, argv);

    if (args.count("help")) {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    if (!args.count("program")) {
        std::cerr << "Must specify a program" << std::endl;
        exit(-1);
    }
    auto programSpec = args["program"].as<std::string>();

    std::shared_ptr<InterpretedProgram>  program;
    if (args.count("block-sizes")) {
        auto sizeSpec = args["block-sizes"].as<std::string>();

        program = std::make_shared<InterpretedProgramFromString>(programSpec, sizeSpec);
    } else {
        Program program2D = Program::fromString(programSpec);

        auto builder = std::make_shared<InterpretedProgramBuilder>();
        builder->buildFromProgram(program2D);

        program = builder;
    }

    program->dumpShortProgram(std::cout);
    std::cout << std::endl;
    program->dump();

    return 0;
}
