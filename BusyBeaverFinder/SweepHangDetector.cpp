//
//  SweepHangDetector.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 10/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "SweepHangDetector.h"

#include <iostream>

#include "ExhaustiveSearcher.h"

SweepHangDetector::SweepHangDetector(ExhaustiveSearcher& searcher) :
    _searcher(searcher)
{
}

int SweepHangDetector::getMaxShiftForLoop(RunBlock* runBlock) {
    RunSummary& runSummary = _searcher.getRunSummary();
    CompiledProgram& compiledProgram = _searcher.getCompiledProgram();
    int maxShift = 0;

    for (int i = 0; i < runBlock->getLoopPeriod(); i++) {
        int blockIndex = runSummary.programBlockIndexAt(i + runBlock->getStartIndex());
        ProgramBlock* programBlock = compiledProgram.getBlock(blockIndex);

        if (! programBlock->isDelta() ) {
            int amount = programBlock->getInstructionAmount();
            if (amount < 0) {
                amount = -amount;
            }
            if (amount > maxShift) {
                maxShift = amount;
            }
        }
    }

    return maxShift;
}

// Determine the maximum shift within either of the sweep loops. If it is larger than one, it may
// mean that some values are skipped during a sweep. This then implies that the hang detection needs
// to analyze more sweeps before it can conclude that all values diverge and there is a hang.
int SweepHangDetector::determineMaxSweepShift() {
//    _searcher.getCompiledProgram().dump();
//    _searcher.dumpHangDetection();

    RunSummary& runSummary = _searcher.getRunSummary();
    int maxShift = 0;

    int numRunBlocks = runSummary.getNumRunBlocks();
    for (int i = numRunBlocks - 4; i < numRunBlocks; i++) {
        RunBlock* runBlock = runSummary.runBlockAt(i);

        if (runBlock->isLoop()) {
            int shift = getMaxShiftForLoop(runBlock);
            if (shift > maxShift) {
                maxShift = shift;
            }
        }
    }

    return maxShift;
}

void SweepHangDetector::start() {
    // Check basic assumption. The meta-run summary should be in a loop with period four:
    // sweep right loop, right reversal block, sweep left loop, left reversal block
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
//    _searcher.dumpHangDetection();

    if ( !metaRunSummary.isInsideLoop() || (metaRunSummary.getLoopPeriod() % 4) != 0 ) {
        _status = HangDetectionResult::FAILED;
    } else {
        _status = HangDetectionResult::ONGOING;
        _metaLoopIndex = metaRunSummary.getNumRunBlocks();
    }

    _sweepCount = 0;
    _midSequenceReveralDp = nullptr;
    _dataPointerAtLoopStart = nullptr;
    _dataBoundary = nullptr;

    _maxSweepShift = 0; // Lazily set it
}

// Should be invoked after a full sweep has been completed. It returns "true" if the data value
// changes are diverging and therefore, the program is considered hanging.
bool SweepHangDetector::isSweepDiverging() {
//    std::cout << "Checking for sweep hang " << std::endl;
//    _searcher.getData().dump();
//    _searcher.getDataTracker().dump();

    Data& data = _searcher.getData();
    DataTracker& dataTracker = _searcher.getDataTracker();

    if (_midSequenceReveralDp != nullptr) {
        long dataIndex = _midSequenceReveralDp - data.getDataBuffer();
        if (
            *_midSequenceReveralDp != dataTracker.getOldSnapShot()->buf[dataIndex]
        ) {
            // The value of the mid-turning point should be unchanged since the last reversal at
            // this side of the sequence. Note, it may have different values during the left and
            // right reversal.
            return false;
        }
    }

    return dataTracker.sweepHangDetected(_midSequenceReveralDp);
}

void SweepHangDetector::checkSweepContract() {
    if (!isSweepDiverging()) {
        // Values do not diverge so we cannot conclude it is a hang.
        _status = HangDetectionResult::FAILED;
        return;
    }

    // We should at least sample each different sweep-loop
    int numSweepsToSample = _searcher.getMetaRunSummary().getLoopPeriod() / 2;

    // The amount of sweeps to check depends on the maximum amount that DP shifts within a sweep
    // loop. Values that are skipped during one sweep might impact the next sweep (when they are
    // actually being evaluated).
    if (_maxSweepShift == 0) {
        _maxSweepShift = determineMaxSweepShift();
    }
    if (_maxSweepShift * 2 > numSweepsToSample) {
        numSweepsToSample = _maxSweepShift * 2;
    }

    int numSweepsSampled = _sweepCount - 1;
    if (numSweepsSampled < numSweepsToSample) {
        // Not yet sufficient sweeps to conclude that values always diverge (so that it is a hang)
        return;
    }

    Data& data = _searcher.getData();
    if (_maxSweepShift >= (data.getMaxVisitedP() - data.getMinVisitedP())) {
        // The sequence is too short. This may be a glider instead.
        _status = HangDetectionResult::FAILED;
        return;
    }

    _status = HangDetectionResult::HANGING;
    return;
}

void SweepHangDetector::signalLoopExit() {
    if (_searcher.getMetaRunSummary().getNumRunBlocks() != _metaLoopIndex) {
        // We exited the assumed endless sweep meta-loop
        _status = HangDetectionResult::FAILED;
        return;
    }

    if (_sweepCount < 2) {
        Data& data = _searcher.getData();
        DataPointer dp = data.getDataPointer();

        _reversalDp[_sweepCount] = dp;

        if (dp > data.getMinBoundP() && dp < data.getMaxBoundP()) {
            if (_midSequenceReveralDp != nullptr) {
                // There can be at most one mid-sequence reversal point.
                _status = HangDetectionResult::FAILED;
                return;
            }

            _midSequenceReveralDp = dp;
        }
    } else {
        checkSweepContract();
    }

    if (_status == HangDetectionResult::ONGOING) {
        _sweepCount++;
        _searcher.getDataTracker().captureSnapShot();
        _dataPointerAtLoopStart = nullptr;
        _dataBoundary = nullptr;
    }
}

void SweepHangDetector::signalLoopIterationCompleted() {
    Data& data = _searcher.getData();
    DataPointer newDp = data.getDataPointer();

    if (_dataPointerAtLoopStart != nullptr) {
        int dpShift = (int)(newDp - _dataPointerAtLoopStart);

        if (dpShift == 0) {
            // DP should shift each iteration
            _status = HangDetectionResult::FAILED;
            return;
        }

        DataPointer dataBoundary = (dpShift < 0) ? data.getMinBoundP() : data.getMaxBoundP();

        if (_dataBoundary != nullptr) {
            if (_dataBoundary != dataBoundary) {
                // The sweep loop should not extend the data boundary
                _status = HangDetectionResult::FAILED;
                return;
            }
        } else {
            _dataBoundary = dataBoundary;
        }
    }
    _dataPointerAtLoopStart = newDp;
}

HangDetectionResult SweepHangDetector::detectHang() {
    // Actual check is done whenever a loop is exited
    return _status;
}
