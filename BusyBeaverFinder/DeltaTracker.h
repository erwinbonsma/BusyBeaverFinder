//
//  DeltaTracker.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef DeltaTracker_h
#define DeltaTracker_h

#include <stdio.h>

class ProgramBlock;

class DeltaTracker {
    int _maxInc;
    int _maxDec;
    int _maxShr;
    int _maxShl;

public:
    DeltaTracker();

    int getMaxInc() { return _maxInc; }
    int getMaxDec() { return _maxDec; }
    int getMaxShr() { return _maxShr; }
    int getMaxShl() { return _maxShl; }

    void reset();
    void update(ProgramBlock* block);

    void dump();
};

#endif /* DeltaTracker_h */
