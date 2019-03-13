//
//  NoExitHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 13/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "NoExitHangDetector.h"

#include "ExhaustiveSearcher.h"
#include "Utils.h"

NoExitHangDetector::NoExitHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
    // void
}

void NoExitHangDetector::start() {
    _startPp = _searcher.getProgramPointer();
}

bool NoExitHangDetector::canExitFrom(ProgramPointer pp) {
    return true;
}

HangDetectionResult NoExitHangDetector::detectHang() {
    ProgramPointer pp = _searcher.getProgramPointer();

    if ( *(pp.p) == Ins::DATA ) {
        if (!canExitFrom(pp)) {
            // Cannot reach new instructions (nor leave the program grid)
            return HangDetectionResult::HANGING;
        } else {
            // Cannot conclude it's a hang
            return HangDetectionResult::FAILED;
        }
    }

    if (PROGRAM_POINTERS_MATCH(_startPp, pp)) {
        // Back to starting point without encountering a DATA instruction, so this is a simple loop
        return HangDetectionResult::HANGING;
    } else {
        return HangDetectionResult::ONGOING;
    }
}
