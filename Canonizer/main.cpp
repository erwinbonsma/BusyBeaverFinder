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

InterpretedProgramCanonizer canonizeProgram(const InterpretedProgram& program) {
    std::string specBefore = program.shortProgramString();
    std::cout << std::endl << "before: " << specBefore << std::endl;

    InterpretedProgramCanonizer canonizer {program};
    std::string specAfter = canonizer.shortProgramString();
    std::cout << "after:  " << specAfter << std::endl;

    canonizer.dumpRaw();

    // Recurse if needed. Merging of duplicate blocks can make other blocks (that jumped to the
    // merged blocks) identical and therefore can be merged as well.
    if (specAfter != specBefore) {
        canonizer = canonizeProgram(canonizer);
        canonizer.dumpRaw();
    }

    return canonizer;
}

void canonizeProgram(std::string& programSpec) {
    Program program = Program::fromString(programSpec);

    InterpretedProgramBuilder builder;
    builder.buildFromProgram(program);

    std::cout << programSpec;
    if (SKIP_CANONIZE) {
        outputInterpretedProgram(builder);
    } else {
        InterpretedProgramCanonizer canonizer = canonizeProgram(builder);
        canonizer.dumpRaw();
        outputInterpretedProgram(canonizer);
    }
}

int main(int argc, char * argv[]) {
    std::string programSpec {"d769ACPUzlMxbIWzq8"};

    canonizeProgram(programSpec);

//    std::string programSpec;
//
//    while (std::getline(std::cin, programSpec)) {
//        canonizeProgram(programSpec);
//    }

    return 0;
}
