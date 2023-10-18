//
//  HangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 14/01/20.
//  Copyright © 2020 Erwin. All rights reserved.
//
#pragma once

#include "Types.h"
#include "ExhaustiveSearcher.h"
#include "RunSummary.h"

class HangDetector {
    // When last hang proof attempt was done that failed
    int _lastFailedCheckPoint;

    // When last analysis was performed
    int _analysisCheckPoint;

protected:
    const ExecutionState& _execution;

    // Returns the current check point, typically the program-block or run-block of either the run
    // summary or meta-run summary. Once the detector checked for a hang (and this checked failed)
    // it will only check again after the check point has changed. So the checkpoint should only
    // change when the next check might pass. What a suitable checkpoint is depends on the type of
    // hang that is being detected.
    int currentCheckPoint() { return _execution.getRunSummary().getNumRunBlocks(); }

    virtual bool shouldCheckNow(bool loopContinues) const = 0;

    // Checks if the run summary and possibly meta-run summary exhibits the characteristic behaviour
    // for the hang that is being detected. Returns true iff this is the case. It should then also
    // have analysed the loop(s) that comprise the assumed hang.
    virtual bool analyzeHangBehaviour() = 0;

    // Clears results of analysis. It is invoked when a hang check failed.
    virtual void clearAnalysis() { _analysisCheckPoint = -1; }

    // Returns true if results of a previous analysis are still available. This is the case when the
    // program might hang, but this cannot yet be proven. When analysing the hang behavior in
    // analyzeHangBehavior a hang detector may want to re-use the old analysis (and verify that it
    // is still valid) instead of redoing it. This is not only a possible optimization, but may be
    // essential to prove certain hangs. In particular, it is needed when the hang cannot be proven
    // in one go and the proof spans multiple checkpoints. This is for example the case for
    // irregular sweep hangs, as these lack natural check points (due to their irregularity).
    bool oldAnalysisAvailable() const { return _analysisCheckPoint != -1; }

    // Checks if it can be proven that the program hang. Returns YES if this is the case. Can
    // return MAYBE if it is not yet clear yet but a later check _at the same check point_ may
    // succeed.
    virtual Trilian proofHang() = 0;

public:
    HangDetector(const ExecutionState& execution);
    virtual ~HangDetector() {}

    virtual HangType hangType() const { return HangType::UNKNOWN; };

    virtual void reset();

    // Returns true if a hang is detected. It should only be executed when the program is inside a
    // loop. The loopContinues flag signals if the loop will also continue. This flag is useful as
    // some hang detectors want the loop to continue (e.g. simple periodic hangs), whereas in case
    // of nested loops, termination of an inner-loop is often a good synchronization point to
    // initiate a check
    bool detectHang(bool loopContinues);
};
