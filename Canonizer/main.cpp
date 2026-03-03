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


void canonizeProgram(std::string& programSpec) {
    Program program = Program::fromString(programSpec);

    InterpretedProgramBuilder builder;
    builder.buildFromProgram(program);

    InterpretedProgramCanonizer canonizer {builder};

    std::cout << canonizer.canonicalProgramString();
    std::cout << "\t" << programSpec << std::endl;
}

int main(int argc, char * argv[]) {
    std::string programSpec{"d++vWSlUsmvlvlv1v8"};

    while (std::getline(std::cin, programSpec)) {
        canonizeProgram(programSpec);
    }

    return 0;
}
