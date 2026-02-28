//
//  Resumer.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 28/02/2026.
//  Copyright © 2026 Erwin. All rights reserved.
//

#include "Resumer.h"

ResumeFromProgram::ResumeFromProgram(const std::string& programSpec)
: _program(Program::fromString(programSpec)) {
    _popCount = 0;

    for (int8_t col = _program.getSize().width; --col >= 0; ) {
        for (int8_t row = _program.getSize().height; --row >= 0; ) {
            Ins ins = _program.getInstruction({col, row});
            if (ins != Ins::UNSET) {
                _popCount++;
            }
        }
    }
}

Ins ResumeFromProgram::popNextInstruction(InstructionPointer ip) {
    _popCount--;
    return _program.getInstruction(ip);
}
