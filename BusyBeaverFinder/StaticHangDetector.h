//
//  StaticHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef StaticHangDetector_h
#define StaticHangDetector_h

#include <stdio.h>

#include "Types.h"

class ExhaustiveSearcher;

enum class DetectionStage : char {
    WAIT_FOR_CHECKPOINT = 0,
    CHECK_BEHAVIOR = 1,
    VERIFY_HANG = 2
};

class StaticHangDetector {
    DetectionStage _stage;

    // When last check was done
    int _lastCheckPoint;

    // When check was done of hang detection that was still indecisive (which can be continued)
    int _ongoingCheckPoint;

protected:
    ExhaustiveSearcher& _searcher;

    // Returns the current check point, typically the program-block or run-block of either the run
    // summary or meta-run summary. Once the detector checked for a hang (and this checked failed)
    // it will only check again after the check point has changed. So the checkpoint should only
    // change when the next check might pass. What a suitable checkpoint is depends on the type of
    // hang that is being detected.
    virtual int currentCheckPoint() = 0;

    // Checks if the run summary and possibly meta-run summary exhibit the characteristic behaviour
    // for the hang that is being detected. Returns "true" iff so.
    virtual Trilian exhibitsHangBehaviour(bool loopContinues) = 0;

    // Checks if it can be proven that the program hang. Returns "true" iff so.
    virtual Trilian canProofHang(bool resumed) = 0;

public:
    StaticHangDetector(ExhaustiveSearcher& searcher);

    virtual HangType hangType() = 0;

    void reset();

    // Returns true if a hang is detected. It should only be executed when an iteration of an
    // (inner) loop just completed. The loopContinues flag signals if the loop will also continue.
    // This flag is useful as some hang detectors want the loop to continue (e.g. simple periodic
    // hangs), whereas in case of nested loops, termination of an inner-loop can is often a good
    // synchronization point to initiate a check
    bool detectHang(bool loopContinues);
};

#endif /* StaticHangDetector_h */
