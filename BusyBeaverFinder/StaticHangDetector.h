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
#include "ExhaustiveSearcher.h"

class StaticHangDetector {
    // When last hang proof attempt was done that failed
    int _lastCheckPoint;

    // When last analysis was performed
    int _analysisCheckPoint;

protected:
    ExhaustiveSearcher& _searcher;

    // Returns the current check point, typically the program-block or run-block of either the run
    // summary or meta-run summary. Once the detector checked for a hang (and this checked failed)
    // it will only check again after the check point has changed. So the checkpoint should only
    // change when the next check might pass. What a suitable checkpoint is depends on the type of
    // hang that is being detected.
    int currentCheckPoint() { return _searcher.getRunSummary().getNumRunBlocks(); }

    virtual bool shouldCheckNow(bool loopContinues) = 0;

    // Checks if the run summary and possibly meta-run summary exhibits the characteristic behaviour
    // for the hang that is being detected. Returns true iff this is the case. It should then also
    // have analysed the loop(s) that comprise the assumed hang.
    virtual bool analyzeHangBehaviour() = 0;

    // Checks if it can be proven that the program hang. Returns YES if this is the case. Can
    // return MAYBE if it is not yet clear yet.
    virtual Trilian proofHang() = 0;

public:
    StaticHangDetector(ExhaustiveSearcher& searcher);
    virtual ~StaticHangDetector() {}

    virtual HangType hangType() = 0;

    virtual void reset();

    // Returns true if a hang is detected. It should only be executed when the program is inside a
    // loop. The loopContinues flag signals if the loop will also continue. This flag is useful as
    // some hang detectors want the loop to continue (e.g. simple periodic hangs), whereas in case
    // of nested loops, termination of an inner-loop is often a good synchronization point to
    // initiate a check
    bool detectHang(bool loopContinues);
};

#endif /* StaticHangDetector_h */
