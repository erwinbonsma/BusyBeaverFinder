//
//  Utils.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef Utils_h
#define Utils_h

#include <stdio.h>
#include <string>
#include <istream>

#include "Types.h"

#define PROGRAM_POINTERS_MATCH(pp1, pp2) ( \
    pp1.p.col == pp2.p.col && \
    pp1.p.row == pp2.p.row && \
    pp1.dir == pp2.dir \
)

bool isPowerOfTwo(int val);
int makePowerOfTwo(int val);

inline int normalizedMod(int operand, int modulus) {
    int val = operand % modulus;
    return (val >= 0) ? val : val + abs(modulus);
}

inline int sign(int val) {
    return val > 0 ? 1 : (val < 0 ? -1 : 0);
}

InstructionPointer nextInstructionPointer(ProgramPointer insP);

void calculateZArray(const char* input, int* output, int len);
int findPeriod(const char* input, int* buf, int len);

// Checks if the "input" array, containing "len" elements, ends with a repeated sequence.
// It will return the length of this sequence if one is found, and zero otherwise.
//
// Examples:
// aaaaabcabc => 3
// abcdabc => 0
// abcdee => 1
int findRepeatedSequence(const int* input, int* buf, int len);

Ins* loadResumeStackFromStream(std::istream &input, int maxSize);

Ins* loadResumeStackFromFile(std::string inputFile, int maxSize);

void dumpDataBuffer(int* buf, int* dataP, int size);
void dumpInstructionStack(Ins* stack);

#endif /* Utils_h */
