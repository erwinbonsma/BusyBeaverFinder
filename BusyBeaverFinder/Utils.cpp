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

    return ch;
}

// Not used anymore, but kept for reference. It helps understanding the findPeriod implementation
// and has its own unit tests, which is possibly useful in troubleshooting.
void calculateZArray(const char* input, int* output, int len) {
    int l, r;
    l = r = 0;

    output[0] = 0; // Should not be used
    for (int i = 1; i < len; i++) {
        if (i > r) {
            l = r = i;
            while (r < len && input[r - l] == input[r]) {
                r++;
            }
            output[i] = r - l;
            r--;
        } else {
            int k = i - l;
            if (output[k] < r - i + 1) {
                // z[k] is less than remaining interval
                output[i] = output[k];
            } else {
                l = i;
                while (r < len && input[r - l] == input[r]) {
                    r++;
                }
                output[i] = r - l;
                r--;
            }
        }
    }
}

int findPeriod(const char* input, int* buf, int len) {
    int l, r;
    l = r = 0;

    buf[0] = 0; // Should not be used
    int z;
    for (int i = 1; i < len; i++) {
        if (i > r) {
            l = r = i;
            while (r < len && input[r - l] == input[r]) {
                r++;
            }
            z = r - l;
            r--;
        } else {
            int k = i - l;
            if (buf[k] < r - i + 1) {
                // z[k] is less than remaining interval
                z = buf[k];
            } else {
                l = i;
                while (r < len && input[r - l] == input[r]) {
                    r++;
                }
                z = r - l;
                r--;
            }
        }

        if (z + i == len) {
            // Done. Found position from where remainder of sequence matches start of sequence
            return i;
        } else {
            // Store z-value. It can potentially be used to determine later z-values (thereby
            // ensuring the algorithm requires a minimal amount of comparisons).
            buf[i] = z;
        }
    }

    return len;
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

void dumpDataBuffer(int* buf, int* dataP, int size) {
    for (int i = 0; i < size; i++) {
        if (&buf[i] == dataP) {
            std::cout << "[";
        }
        std::cout << buf[i];
        if (&buf[i] == dataP) {
            std::cout << "] ";
        } else {
            std::cout << " ";
        }

    }
    std::cout << std::endl;
}
