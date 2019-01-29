//
//  Utils.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#include "Utils.h"

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
