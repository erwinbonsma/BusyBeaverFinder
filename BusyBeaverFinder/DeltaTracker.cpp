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

#include "ProgramBlock.h"

DeltaTracker::DeltaTracker() {
    reset();
}

void DeltaTracker::reset() {
    _maxInc = 0;
    _maxDec = 0;
    _maxShr = 0;
    _maxShl = 0;
}

void DeltaTracker::update(ProgramBlock* block) {
    int amount = block->getInstructionAmount();
    if (block->isDelta()) {
        if (amount > _maxInc) {
            _maxInc = amount;
        } else if (-amount > _maxDec){
            _maxDec = -amount;
        }
    } else {
        if (amount > _maxShr) {
            _maxShr = amount;
        } else if (-amount > _maxShl){
            _maxShl = -amount;
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
