//
//  Consts.h
//  BusyBeaverFinder
//
//  Created by Erwin on 19/01/19.
//  Copyright © 2019 Erwin. All rights reserved.
//

#ifndef Consts_h
#define Consts_h

const int w = 5;
const int h = 5;

const int dataSize = 16384;

const int maxSteps = 1024;
const int hangSamplePeriod = 64;

#define HANG_DETECTION1

// Over-dimension to avoid need for bound checks
const int undoStackSize = maxSteps * 2;

#ifdef HANG_DETECTION1
const int effectiveStackSize = maxSteps * 2;
#endif

#endif /* Consts_h */
