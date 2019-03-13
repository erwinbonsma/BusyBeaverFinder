//
//  NoExitHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 13/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef NoExitHangDetector_h
#define NoExitHangDetector_h

#include <stdio.h>

#include "HangDetector.h"

class ExhaustiveSearcher;

class NoExitHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;

    ProgramPointer _startPp;

    bool canExitFrom(ProgramPointer pp);

public:
    NoExitHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::NO_EXIT; }

    void start();
    HangDetectionResult detectHang();
};

#endif /* NoExitHangDetector_h */
