//
//  DeltaTracker.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 11/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "DeltaTracker.h"

#include <iostream>
#include <assert.h>

#include "ExhaustiveSearcher.h"

DeltaTracker::DeltaTracker(ExhaustiveSearcher& searcher) :
    _searcher(searcher),
    _data(searcher.getData())
{
    reset();
}

void DeltaTracker::reset() {
    _maxInc = 0;
    _maxDec = 0;
    _maxShr = 0;
    _maxShl = 0;

    _initialDp = _data.getDataPointer();
    _initialValue = *_initialDp;
}

void DeltaTracker::update() {
    ProgramPointer pp = _searcher.getProgramPointer();
    DataPointer newDp = _data.getDataPointer();
    int newVal = _data.val();

    if (pp.dir == _curDir) {
        switch (pp.dir) {
            case Dir::RIGHT:
                if (newDp - _initialDp > _maxShr) {
                    _maxShr = (int)(newDp - _initialDp);
                }
                break;
            case Dir::LEFT:
                if (_initialDp - newDp > _maxShl) {
                    _maxShl = (int)(_initialDp - newDp);
                }
                break;
            case Dir::UP:
                if (newVal - _initialValue > _maxInc) {
                    _maxInc = newVal - _initialValue;
                }
                break;
            case Dir::DOWN:
                if (_initialValue - newVal > _maxDec) {
                    _maxDec = _initialValue - newVal;
                }
                break;
        }
    } else {
        _initialValue = newVal;
        _initialDp = newDp;
        _curDir = pp.dir;

        if (_searcher.getProgram().getInstruction(pp.p) == Ins::DATA) {
            // Compensate for TURN, if any.
            switch (_curDir) {
                case Dir::RIGHT: _initialDp--; if (_maxShr == 0) { _maxShr = 1; } break;
                case Dir::LEFT: _initialDp++; if (_maxShl == 0) { _maxShl = 1; } break;
                case Dir::UP: _initialValue--; if (_maxInc == 0) { _maxInc = 1; } break;
                case Dir::DOWN: _initialValue++; if (_maxDec == 0) { _maxDec = 1; } break;
            }
        }
    }
}

void DeltaTracker::dump() {
    std::cout
    << "maxInc = " << _maxInc
    << ", maxDec = " << _maxDec
    << ", maxShr = " << _maxShr
    << ", maxShl = " << _maxShl << std::endl;
}
