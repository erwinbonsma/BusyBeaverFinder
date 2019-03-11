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

#include "Types.h"

class Data;
class ExhaustiveSearcher;

class DeltaTracker {
    ExhaustiveSearcher& _searcher;
    Data& _data;

    Dir _curDir;
    DataPointer _initialDp;
    int _initialValue;

    int _maxInc;
    int _maxDec;
    int _maxShr;
    int _maxShl;

public:
    DeltaTracker(ExhaustiveSearcher& searcher);

    int getMaxInc() { return _maxInc; }
    int getMaxDec() { return _maxDec; }
    int getMaxShr() { return _maxShr; }
    int getMaxShl() { return _maxShl; }

    void reset();
    void update();

    void dump();
};

#endif /* DeltaTracker_h */
