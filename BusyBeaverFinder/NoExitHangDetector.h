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
#include "Program.h"

class ExhaustiveSearcher;

// The maximum number of path starts. It's not directly obvious what the maximum is that can be
// needed in practise, but this should suffice.
const int maxNumPathStarts = maxWidth * maxHeight;

// Flags to track path starts in _followed array
const char dirFlagUp    = 0x01;
const char dirFlagRight = 0x02;
const char dirFlagDown  = 0x04;
const char dirFlagLeft  = 0x08;

struct PathStart {
    ProgramPointer pp;
    bool dataIsZero;
};

class NoExitHangDetector : public HangDetector {

    ExhaustiveSearcher& _searcher;
    Program& _program;

    // Tracks which path have been followed. It will only be set at places where a path can branch
    // and bend (i.e. in front of TURN instructions)
    char _followed[programStorageSize];
    PathStart _pendingStack[maxNumPathStarts];
    PathStart *_nextP;
    PathStart *_topP;
    PathStart *_lastP;

    void addToStack(InstructionPointer insP, Dir dir, bool dataIsZero);
    bool followPath(PathStart pathStart);
    bool canEscapeFrom(ProgramPointer pp);

public:
    NoExitHangDetector(ExhaustiveSearcher& searcher);

    HangType hangType() { return HangType::NO_EXIT; }

    HangDetectionResult detectHang();
};

#endif /* NoExitHangDetector_h */
