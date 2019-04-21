//
//  HangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef HangDetector_h
#define HangDetector_h

#include "Types.h"

enum class HangDetectionResult : char {
    // Detection is still ongoing. It's too early to conclude
    ONGOING = 0,

    // A hang was detected
    HANGING = 1,

    // Failed to detect a hang
    FAILED = 2
};

class HangDetector {
public:
    virtual ~HangDetector() {}

    virtual HangType hangType() = 0;

    virtual void start() {};
    virtual void signalLoopExit() {}
    virtual void signalLoopIterationCompleted() {}
    virtual HangDetectionResult detectHang() = 0;
};

#endif /* HangDetector_h */
