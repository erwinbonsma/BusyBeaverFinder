//
//  NoExitHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 13/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "NoExitHangDetector.h"

#include <assert.h>

#include "ExhaustiveSearcher.h"
#include "Utils.h"

NoExitHangDetector::NoExitHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
    for (int i = 0; i < programStorageSize; i++) {
        _followed[i] = 0;
    }

    _lastP = _pendingStack + maxNumPathStarts;
    _instructionBuf = searcher.getProgram().getInstructionBuffer();
}

void NoExitHangDetector::start() {
    _startPp = _searcher.getProgramPointer();
}

void NoExitHangDetector::addToStack(InstructionPointer insP, Dir dir, bool dataIsZero) {
    char dirFlag = (int)dir < 0
        ? ((dir == Dir::DOWN) ? dirFlagDown : dirFlagLeft )
        : ((dir == Dir::UP) ? dirFlagUp : dirFlagRight);
    if (dataIsZero) {
        dirFlag <<= 4;
    }

//    std::cout << "Add to stack: insP = " << (insP - _instructionBuf)
//    << ", dir = " << (int)dir
//    << ", is_zero = " << dataIsZero << std::endl;

    char* followedP = _followed + (insP - _instructionBuf);
    if ((*followedP & dirFlag) == 0) {
        // Path start is not yet on stack

        *followedP |= dirFlag;
        assert(_topP < _lastP);

        // Add it to the stack
        *_topP++ = PathStart { .pp = { .p = insP, .dir = dir }, .dataIsZero = dataIsZero };
    }
}

bool NoExitHangDetector::followPath(PathStart pathStart) {
    bool encounteredData = false;
    ProgramPointer pp = pathStart.pp;

    while (1) {
        InstructionPointer insP = pp.p + (int)pp.dir;

        switch (*insP) {
            case Ins::DATA:
                encounteredData = true;
                break;
            case Ins::NOOP:
                break;
            case Ins::DONE:
            case Ins::UNSET:
                // Escaped from loop. So cannot conclude that program hangs
                return true;
            case Ins::TURN:
                if (pathStart.dataIsZero || encounteredData) {
                    switch (pathStart.pp.dir) {
                        case Dir::UP: addToStack(pp.p, Dir::LEFT, true); break;
                        case Dir::RIGHT: addToStack(pp.p, Dir::UP, true); break;
                        case Dir::DOWN: addToStack(pp.p, Dir::RIGHT, true); break;
                        case Dir::LEFT: addToStack(pp.p, Dir::DOWN, true); break;
                    }
                }
                if (!pathStart.dataIsZero || encounteredData) {
                    switch (pathStart.pp.dir) {
                        case Dir::UP: addToStack(pp.p, Dir::RIGHT, false); break;
                        case Dir::RIGHT: addToStack(pp.p, Dir::DOWN, false); break;
                        case Dir::DOWN: addToStack(pp.p, Dir::LEFT, false); break;
                        case Dir::LEFT: addToStack(pp.p, Dir::UP, false); break;
                    }
                }
                return false;
        }

        pp.p = insP;
    }
}

bool NoExitHangDetector::canEscapeFrom(ProgramPointer pp) {
    _nextP = _pendingStack;
    _topP = _pendingStack;

    assert( *(pp.p) == Ins::DATA );
    PathStart searchStart = { .pp = pp, .dataIsZero = true };
    bool escapedFromLoop = followPath(searchStart);
    searchStart.dataIsZero = false;
    escapedFromLoop = escapedFromLoop || followPath(searchStart);

    while (!escapedFromLoop && _nextP < _topP) {
        escapedFromLoop = followPath(*_nextP++);
    }

    // Reset tracking state
    while (_topP > _pendingStack) {
        _followed[ (--_topP)->pp.p - _instructionBuf ] = 0;
    }

    return escapedFromLoop;
}

HangDetectionResult NoExitHangDetector::detectHang() {
    ProgramPointer pp = _searcher.getProgramPointer();

    if ( *(pp.p) == Ins::DATA ) {
//        _searcher.dump();
        if (canEscapeFrom(pp)) {
            // Cannot conclude it's a hang
            return HangDetectionResult::FAILED;
        } else {
            // Cannot reach new instructions (nor leave the program grid)
            return HangDetectionResult::HANGING;
        }
    }

    if (PROGRAM_POINTERS_MATCH(_startPp, pp)) {
        // Back to starting point without encountering a DATA instruction, so this is a simple loop
        return HangDetectionResult::HANGING;
    } else {
        return HangDetectionResult::ONGOING;
    }
}
