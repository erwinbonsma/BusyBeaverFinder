//
//  CompiledProgram.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 26/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "CompiledProgram.h"

enum class ProgramBlockFlags : char {
    // Set when instruction has been set
    INSTRUCTION_SET = 0x01,

    // Set when instruction is a Delta (otherwise it is a Shift)
    INSTRUCTION_TYPE = 0x02,

    // Set when the block is finialized
    FINALIZED = 0x04
};
