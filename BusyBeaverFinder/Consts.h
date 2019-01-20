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

const int dataSize = 1024;

const int maxSteps = 65536;
const int hangSamplePeriod = 256;

#define HANG_DETECTION1
#define HANG_DETECTION2

// Over-dimension to avoid need for bound checks
const int undoStackSize = maxSteps * 2;

#ifdef HANG_DETECTION1
const int effectiveStackSize = maxSteps * 2;
#endif

#ifdef HANG_DETECTION2
const int hangDeltaSize  = hangSamplePeriod;
#endif

#endif /* Consts_h */
