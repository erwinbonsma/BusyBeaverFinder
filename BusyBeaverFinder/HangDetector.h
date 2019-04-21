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

    // Signalled when it is detected that a new loop is entered. Note, the actual loop will have
    // started earlier, as it requires two loop iterations to detect that execution is in a loop.
    virtual void signalLoopStartDetected() {}

    // Signalled when it is detected that the loop is exited. It is signalled just before the
    // instruction that follows the loop is executed.
    virtual void signalLoopExit() {}

    // Signalled when the current loop completed a full iteration. During loop execution it is
    // repeatedly executed, always at the same moment in the loop, which makes it very suitable for
    // checking that the loop behaves how it is supposed to, given the type of hang that is assumed.
    virtual void signalLoopIterationCompleted() {}

    virtual HangDetectionResult detectHang() = 0;
};

#endif /* HangDetector_h */
