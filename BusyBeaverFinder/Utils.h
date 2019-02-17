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

#include "Types.h"

bool isPowerOfTwo(int val);
int makePowerOfTwo(int val);

void calculateZArray(const char* input, int* output, int len);
int findPeriod(const char *input, int* buf, int len);

Ins* loadResumeStackFromFile(std::string inputFile, int maxSize);

void dumpDataBuffer(int* buf, int* dataP, int size);

#endif /* Utils_h */
