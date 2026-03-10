//
//  Canonizer.cpp
//  Canonizer
//
//  Created by Erwin on 03/03/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include <string>
#include <iostream>

#include "Program.h"
#include "InterpretedProgramBuilder.h"
#include "InterpretedProgramCanonizer.h"


// Can be disabled for debugging/sanity checks
constexpr bool SKIP_CANONIZE = false;


void outputInterpretedProgram(InterpretedProgram& program) {
    std::cout << "\t" << program.shortProgramString();
    std::cout << "\t" << program.blockSizeString() << std::endl;
}

void canonizeProgram(std::string& programSpec) {
    Program program = Program::fromString(programSpec);

    InterpretedProgramBuilder builder;
    builder.buildFromProgram(program);

    std::cout << programSpec;
    if (SKIP_CANONIZE) {
        outputInterpretedProgram(builder);
    } else {
        InterpretedProgramCanonizer canonizer {builder};
        outputInterpretedProgram(canonizer);
    }
}

int main(int argc, char * argv[]) {
    std::string programSpec;

    while (std::getline(std::cin, programSpec)) {
        canonizeProgram(programSpec);
    }

    return 0;
}
