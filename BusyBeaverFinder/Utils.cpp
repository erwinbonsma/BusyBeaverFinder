//
//  Utils.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "Utils.h"

#include <assert.h>
#include <iostream>

bool isPowerOfTwo(int val) {
    return (val & (val - 1)) == 0;
}

int makePowerOfTwo(int val) {
    int shifts = 0;
    while (val != 0) {
        shifts ++;
        val >>= 1;
    }
    return 1 << shifts;
}

int readNextChar(FILE* file) {
    int ch;
    bool skip = false;

    do {
        ch = getc(file);

        while (ch == '#') {
            // Swallow characters until EOL
            do {
                std::cout << (char)ch;
                ch = getc(file);
            } while (ch != 10 && ch != EOF);
            std::cout << std::endl;
        }

        // Ignore whitespace
        skip = (isspace(ch) || ch == 10);
    } while (skip);

    std::cout << (char)ch;
    return ch;
}

Op* loadResumeStackFromFile(std::string inputFile, int maxSize) {
    Op* resumeStack = new Op[maxSize];
    int numOps = 0;

    std::cout << "Resuming from " << inputFile << std::endl;
    FILE *file = fopen(inputFile.c_str(), "r");
    if (file) {
        int ch;
        while (numOps < maxSize - 1 && (ch = readNextChar(file)) != EOF) {
            int intVal = ch - '0';
            assert(intVal >= 0 && intVal <= 3);
            resumeStack[numOps++] = (Op)intVal;

            ch = readNextChar(file);
            assert(ch == ',' || ch == EOF);
        }
        fclose(file);
    }
    else {
        std::cout << "Could not read file" << std::endl;
    }
    resumeStack[numOps] = Op::UNSET; // Add UNSET operation as guard

    return resumeStack;
}
