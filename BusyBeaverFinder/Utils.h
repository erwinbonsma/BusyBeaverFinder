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

#include "Enums.h"

bool isPowerOfTwo(int val);
int makePowerOfTwo(int val);

Op* loadResumeStackFromFile(std::string inputFile, int maxSize);

#endif /* Utils_h */
