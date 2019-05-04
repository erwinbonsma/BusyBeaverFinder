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
    InterpretedProgram& interpretedProgram = _searcher.getInterpretedProgram();
    int maxShift = 0;

    for (int i = 0; i < runBlock->getLoopPeriod(); i++) {
        int blockIndex = runSummary.programBlockIndexAt(i + runBlock->getStartIndex());
        ProgramBlock* programBlock = interpretedProgram.getEntryBlock() + blockIndex;

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
    RunSummary& runSummary = _searcher.getRunSummary();
    int maxShift = 0;

    int numRunBlocks = runSummary.getNumRunBlocks();
    for (int i = numRunBlocks - _metaLoopPeriod; i < numRunBlocks; i++) {
        RunBlock* runBlock = runSummary.runBlockAt(i);

        if (runBlock->isLoop() && !shouldIgnoreLoop(runBlock->getSequenceIndex())) {
            int shift = getMaxShiftForLoop(runBlock);
            if (shift > maxShift) {
                maxShift = shift;
            }
        }
    }

    return maxShift;
}

// The basic sweep patterns is: [Sweep Loop 1] (Reversal 1) [Sweep Loop 2] (Reversal 2)
// Furthermore, the iterations of the sweep loops should increase over time, as the sequence to
// sweep gets longer.
//
// Variations:
// - It can happen that a Reversal sequences contains a fixed loop. If so it should be ignored by
//   this hang detector and considered part of the reveral sequences.
// - The transition from one loop to the other may be without a reversal sequence.
bool SweepHangDetector::isSweepLoopPattern() {
    RunSummary& metaRunSummary = _searcher.getMetaRunSummary();
    RunSummary& runSummary = _searcher.getRunSummary();

    if (!metaRunSummary.isInsideLoop()) {
        return false;
    }

    _metaLoopIndex = metaRunSummary.getNumRunBlocks();
    _metaLoopPeriod = metaRunSummary.getLoopPeriod();

    if (_metaLoopPeriod < 2) {
        return false;
    }

    _numSweepLoops = 0;
    _numNonSweepLoopsToIgnore = 0;

    // Skip the last, yet unfinalized, run block
    int startIndex = runSummary.getNumRunBlocks() - 2;
    for (int i = 0; i < _metaLoopPeriod; i++) {
        int idx1 = startIndex - i;
        RunBlock* runBlock1 = runSummary.runBlockAt(idx1);

        if (runBlock1->isLoop()) {
            int idx2 = idx1 - _metaLoopPeriod;
            int len1 = runSummary.getRunBlockLength(idx1);
            int len2 = runSummary.getRunBlockLength(idx2);

            if (runBlock1->getSequenceIndex() != runSummary.runBlockAt(idx2)->getSequenceIndex()) {
                // Can happen when the meta-run loop was just detected. In that case, idx2 may just
                // be outside the loop.
                return false;
            }

            if (len1 > len2) {
                _numSweepLoops++;
            }
            else if (len1 == len2) {
                // A fixed loop, which should be ignored

                if (_numNonSweepLoopsToIgnore >= maxNonSweepLoopsToIgnore) {
                    // More non-sweep loops than expected. (Note: limit should be bumped when
                    // needed).
                    return false;
                }

                _nonSweepLoopIndexToIgnore[_numNonSweepLoopsToIgnore++] =
                    runBlock1->getSequenceIndex();
            } else {
                // This never happens during a sweep hang
                return false;
            }
        }
    }

    if (_numSweepLoops < 2 || (_numSweepLoops % 2) != 0) {
        return false;
    }

    // Check if the hard-coded limits on the data structures used by the detector are sufficient
    assert(_numSweepLoops <= maxSweepsToSample);

    _ignoreCurrentLoop = shouldIgnoreCurrentLoop();

    return true;
}

HangDetectionResult SweepHangDetector::start() {
    if ( !isSweepLoopPattern() ) {
        return HangDetectionResult::FAILED;
    }

    _sweepCount = 0;
    _indexOfFirstMidSweepReversal = -1;
    _dataPointerAtLoopStart = nullptr;
    _dataBoundary = nullptr;

    _maxSweepShift = 0; // Lazily set it

    return HangDetectionResult::ONGOING;
}

// Should be invoked after a full sweep has been completed. It returns "true" if the data value
// changes are diverging and therefore, the program is considered hanging.
bool SweepHangDetector::isSweepDiverging() {
//    std::cout << "Checking for sweep hang " << std::endl;
//    _searcher.getData().dump();
//    _searcher.getDataTracker().dump();

    DataTracker& dataTracker = _searcher.getDataTracker();

    if (_sweepReversalPoint[_sweepCount].midSequence && _sweepCount >= _numSweepLoops) {
        // Check that the mid-sweep point diverges. It compares the value at the end of each sweep
        // loop, compared to the value as it was at the end of the same sweep loop, during the
        // previous iteration of the meta-run loop

        SweepReversalPoint& old = _sweepReversalPoint[_sweepCount - _numSweepLoops];
        SweepReversalPoint& cur = _sweepReversalPoint[_sweepCount];

        if (old.dp != cur.dp) {
            // Position of mid-sequence reversal should be fixed
            return false;
        }

        if (
            (old.value != cur.value) && (
                (old.value == 0) ||
                (cur.value == 0) ||
                ((old.value < 0) && (cur.value > old.value)) ||
                ((old.value > 0) && (cur.value < old.value))
            )
        ) {
            // Non-diverging change to mid-sequence data point
            return false;
        }
    }

    int i = _sweepCount;
    while (i >= 0 && !_sweepReversalPoint[i].midSequence) {
        i--;
    }

    return dataTracker.sweepHangDetected(i >= 0 ? _sweepReversalPoint[i].dp : nullptr);
}

HangDetectionResult SweepHangDetector::checkSweepContract() {
//    _searcher.dumpHangDetection();

    if (!isSweepDiverging()) {
        // Values do not diverge so we cannot conclude it is a hang.
        return HangDetectionResult::FAILED;
    }

    // We should at least sample each different sweep-loop
    int numSweepsToSample = _numSweepLoops;

    if (_indexOfFirstMidSweepReversal != -1) {
        // We need to check each loop twice, to ensure the mid-sweep point diverges
        numSweepsToSample *= 2;
    }

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
        return HangDetectionResult::ONGOING;
    }

    Data& data = _searcher.getData();
    if (_maxSweepShift >= (data.getMaxVisitedP() - data.getMinVisitedP())) {
        // The sequence is too short. This may be a glider instead.
        return HangDetectionResult::FAILED;
    }

    return HangDetectionResult::HANGING;
}


bool SweepHangDetector::shouldIgnoreLoop(int sequenceIndex) {
    for (int i = 0; i < _numNonSweepLoopsToIgnore; i++) {
        if (_nonSweepLoopIndexToIgnore[i] == sequenceIndex) {
            return true;
        }
    }

    return false;
}

bool SweepHangDetector::shouldIgnoreCurrentLoop() {
    return shouldIgnoreLoop( _searcher.getRunSummary().getLastRunBlock()->getSequenceIndex() );
}

HangDetectionResult SweepHangDetector::signalLoopStartDetected() {
    _ignoreCurrentLoop = shouldIgnoreCurrentLoop();
    return HangDetectionResult::ONGOING;
}

HangDetectionResult SweepHangDetector::signalLoopExit() {
    if (_ignoreCurrentLoop) {
        return HangDetectionResult::ONGOING;
    }

    if (_searcher.getMetaRunSummary().getNumRunBlocks() != _metaLoopIndex) {
        // We exited the assumed endless sweep meta-loop
        return HangDetectionResult::FAILED;
    }

    Data& data = _searcher.getData();
    DataPointer dp = data.getDataPointer();

    // Record the reversal point
    SweepReversalPoint& reversalPoint = _sweepReversalPoint[_sweepCount];
    reversalPoint.dp = dp;
    reversalPoint.value = *dp;
    reversalPoint.midSequence =  dp > data.getMinBoundP() && dp < data.getMaxBoundP();

    if (reversalPoint.midSequence) {
        if (_indexOfFirstMidSweepReversal != -1) {
            if (_indexOfFirstMidSweepReversal % 2 != _sweepCount % 2) {
                // Mid-sweep reversal should only occur at one end of the sequence
                return HangDetectionResult::FAILED;
            }
        } else {
            _indexOfFirstMidSweepReversal = _sweepCount;
        }
    }

    if (_sweepCount >= 2) {
        HangDetectionResult result = checkSweepContract();
        if (result != HangDetectionResult::ONGOING) {
            return result;
        }
    }

    _sweepCount++;
    assert(_sweepCount < maxSweepsToSample);
    _searcher.getDataTracker().captureSnapShot();
    _dataPointerAtLoopStart = nullptr;
    _dataBoundary = nullptr;

    return HangDetectionResult::ONGOING;
}

HangDetectionResult SweepHangDetector::signalLoopIteration() {
    if (_ignoreCurrentLoop) {
        return HangDetectionResult::ONGOING;
    }

    Data& data = _searcher.getData();
    DataPointer newDp = data.getDataPointer();

    if (_dataPointerAtLoopStart != nullptr) {
        int dpShift = (int)(newDp - _dataPointerAtLoopStart);

        if (dpShift == 0) {
            // DP should shift each iteration
            return HangDetectionResult::FAILED;
        }

        DataPointer dataBoundary = (dpShift < 0) ? data.getMinBoundP() : data.getMaxBoundP();

        if (_dataBoundary != nullptr) {
            if (_dataBoundary != dataBoundary) {
                // The sweep loop should not extend the data boundary
                return HangDetectionResult::FAILED;
            }
        } else {
            _dataBoundary = dataBoundary;
        }
    }
    _dataPointerAtLoopStart = newDp;

    return HangDetectionResult::ONGOING;
}
