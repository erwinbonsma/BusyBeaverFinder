//
//  Consts.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#ifndef Consts_h
#define Consts_h

const int w = 7;
const int h = 7;

const int dataSize = 16384;

const int maxSteps = 16384 * 4;
const int earlyHangCheck = 256;

// Over-dimension to avoid need for bound checks
const int undoStackSize = maxSteps * 2;
const int effectiveStackSize = maxSteps * 2;

#endif /* Consts_h */
