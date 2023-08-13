//
//  ExhaustiveSearcher.cpp
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#include "ExhaustiveSearcher.h"

#include <iostream>
#include <assert.h>

#include "Utils.h"

#include "GliderHangDetector.h"
#include "MetaPeriodicHangDetector.h"
#include "PeriodicSweepHangDetector.h"
#include "IrregularSweepHangDetector.h"

Ins validInstructions[] = { Ins::NOOP, Ins::DATA, Ins::TURN };

Ins targetStack[] = {
    // 5x5 InfSeqExtendingBothWays2
    Ins::DATA, Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::DATA, Ins::TURN, Ins::DATA,
    Ins::TURN, Ins::DATA, Ins::TURN, Ins::NOOP, Ins::TURN, Ins::DATA, Ins::TURN, Ins::UNSET
};

const ProgramPointer backtrackProgramPointer =
    ProgramPointer { .p = InstructionPointer { .col = -1, .row = -1 }, .dir = Dir::UP };

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, SearchSettings settings) :
    _settings(settings),
    _program(width, height),
    _exitFinder(_program, _programBuilder),
    _hangExecutor(settings.dataSize, settings.maxHangDetectionSteps),
    _fastExecutor(settings.dataSize)
{
    initInstructionStack(width * height);

    _fastExecutor.setMaxSteps(_settings.maxSteps);
    _hangExecutor.setMaxSteps(_settings.maxSteps);

    // Set default search mode
    _searchMode = SearchMode::FULL_TREE;
    _delayHangDetection = false;
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;
}

void ExhaustiveSearcher::initInstructionStack(int size) {
    _instructionStack = new Ins[size];

    for (int i = size; --i >= 0; ) {
        _instructionStack[i] = Ins::UNSET;
    }
}

bool ExhaustiveSearcher::instructionStackEquals(Ins* reference) const {
    Ins* p1 = _instructionStack;
    Ins* p2 = reference;

    while (*p1 != Ins::UNSET && *p2 != Ins::UNSET && *p1 == *p2) {
        p1++;
        p2++;
    }

    return (*p1 == *p2);
}

bool ExhaustiveSearcher::atTargetProgram() {
    return instructionStackEquals(targetStack);
}

void ExhaustiveSearcher::dumpInstructionStack() const {
    ::dumpInstructionStack(_instructionStack);
}

void ExhaustiveSearcher::dumpSettings() {
    std::cout
    << "Size = " << _program.getWidth() << "x" << _program.getHeight()
    << ", DataSize = " << _settings.dataSize
    << ", MaxSteps = " << _settings.maxHangDetectionSteps << "/" << _settings.maxSteps
    << ", UndoCapacity = " << _settings.undoCapacity
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
}

void ExhaustiveSearcher::setProgressTracker(ProgressTracker* tracker) {
    _tracker = tracker;
}

void ExhaustiveSearcher::branch(int depth) {
    ProgramPointer pp0 = _pp;
    InstructionPointer insP = nextInstructionPointer(_pp);
    bool resuming = *_resumeFrom != Ins::UNSET;
    bool abortSearch = (
        _searchMode == SearchMode::FIND_ONE || (_searchMode == SearchMode::SUB_TREE && resuming)
    );

    if (!resuming && !_data.undoEnabled()) {
        // First unset instruction after the resumeFrom stack

        // Enable backtracking
        _data.setEnableUndo(true);

        if (_delayHangDetection) {
            // Start hang detection
            _delayHangDetection = false;
            _hangDetectionEnd += _numSteps;
        }
    }

    for (int i = 0; i < 3; i++) {
        Ins ins = validInstructions[i];

        if (resuming) {
            if (ins == *_resumeFrom) {
                _resumeFrom++;

                resuming = false; // Let search continue in FULL_TREE search mode
            } else {
                continue;
            }
        }

        _program.setInstruction(insP, ins);
        _instructionStack[depth] = ins;
//        if (atTargetProgram()) {
//            std::cout << "At target!" << std::endl;
//        }
        run(depth + 1);
        _pp = pp0;

        if (abortSearch) {
            assert(_data.getUndoStackSize() == 0);
            break;
        }
    }
    _program.clearInstruction(insP);
    _instructionStack[depth] = Ins::UNSET;
}

void ExhaustiveSearcher::fastExecution() {
    _tracker->reportFastExecution();
    RunResult result = _fastExecutor.execute(_programBuilder.getEntryBlock());
    switch (result) {
        case RunResult::SUCCESS:
        case RunResult::PROGRAM_ERROR:
            _tracker->reportLateEscape(_fastExecutor.numSteps());
            break;
        case RunResult::DATA_ERROR:
            _tracker->reportError();
            break;
        case RunResult::ASSUMED_HANG:
            _tracker->reportAssumedHang();
            break;
        case RunResult::UNKNOWN:
        case RunResult::DETECTED_HANG:
            assert(false);
    }
}

// Returns "true" if the search should backtrack. This can be for several reason: the program exited
// with an error (as it ran out of data tape), the undo buffer is full, or a hang is assumed (given
// the amount of steps executed)
bool ExhaustiveSearcher::executeCurrentBlock() {
    int amount = _block->getInstructionAmount();
    if (_block->isDelta()) {
        if (amount > 0) {
            _data.inc(amount);
        } else {
            _data.dec(-amount);
        }
    } else {
        if (amount > 0) {
            if (!_data.shr(amount)) {
                _tracker->reportError();
                return true;
            }
        } else {
            if (!_data.shl(-amount)) {
                _tracker->reportError();
                return true;
            }
        }
    }

    _numSteps += _block->getNumSteps();

    if (_numSteps >= _settings.maxSteps) {
        _tracker->reportAssumedHang();
        return true;
    }

    if (!_data.hasUndoCapacity()) {
        fastExecution();
        return true;
    }

    _block = (_data.val() == 0) ? _block->zeroBlock() : _block->nonZeroBlock();
    return false;
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocksWithBacktracking() {
    while (_block->isFinalized()) {
        if (executeCurrentBlock()) {
            return backtrackProgramPointer;
        }
    }

    _programBuilder.enterBlock(_block);
    return _programBuilder.getStartProgramPointer(_block, _program);
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocksWithHangDetection() {
    const ProgramBlock* entryBlock = _programBuilder.getEntryBlock();

    _runSummary[0].reset();
    _runSummary[1].reset();

    for (auto hangDetector : _hangDetectors) {
        hangDetector->reset();
    }

    while (_block->isFinalized()) {
        // Record block before executing it. This way, when signalling a loop exit, the value
        // that triggered this, which typically is zero, is still present in the data values.
        if (true) {
            // Track execution while hang detection is still active
            int numRunBlocks = _runSummary[0].getNumRunBlocks();

            if (_runSummary[0].recordProgramBlock((int)(_block - entryBlock))) {
                for (int i = numRunBlocks; i < _runSummary[0].getNumRunBlocks(); i++) {
                    const RunBlock* runBlock = _runSummary[0].runBlockAt(i);
                    _runSummary[1].recordProgramBlock(runBlock->getSequenceIndex());
                }

                if (!_runSummary[0].hasSpaceRemaining()) {
                    // Disable hang detection as history buffer is full
                    return executeCompiledBlocksWithBacktracking();
                }
            }
        }

        if (executeCurrentBlock()) {
            return backtrackProgramPointer;
        }

        if (_runSummary[0].isInsideLoop()) {
            bool loopContinues = _runSummary[0].loopContinues((int)(_block - entryBlock));
            for (auto hangDetector : _hangDetectors) {
                if (hangDetector->detectHang(loopContinues)) {
                    bool testHang = _settings.testHangDetection && (
                        hangDetector->hangType() == HangType::REGULAR_SWEEP ||
                        hangDetector->hangType() == HangType::IRREGULAR_SWEEP ||
                        hangDetector->hangType() == HangType::APERIODIC_GLIDER
                    );

                    _tracker->reportDetectedHang(hangDetector, testHang);
                    if (testHang) {
                        fastExecution();
                    }
                    return backtrackProgramPointer;
                }
            }
        }

        if (_numSteps >= _hangDetectionEnd) {
            return executeCompiledBlocks();
        }
    }

    _programBuilder.enterBlock(_block);
    return _programBuilder.getStartProgramPointer(_block, _program);
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocks() {
    if (!_data.hasUndoCapacity()) {
        fastExecution();
        return backtrackProgramPointer;
    }

    if (_numSteps >= _hangDetectionEnd || _delayHangDetection) {
        return executeCompiledBlocksWithBacktracking();
    }

    return executeCompiledBlocksWithHangDetection();
}

void ExhaustiveSearcher::run(int depth) {
    const UndoOp* initialDataUndoP = _data.getUndoStackPointer();
    int initialSteps = _numSteps;

    _programBuilder.push();

    while (1) { // Run until branch, termination or error
processInstruction:
        InstructionPointer insP = nextInstructionPointer(_pp);

        switch (_program.getInstruction(insP)) {
            case Ins::DONE:
                _tracker->reportDone(_numSteps + 1);
                goto backtrack;
            case Ins::UNSET:
                branch(depth);
                goto backtrack;
            case Ins::NOOP:
                break;
            case Ins::DATA:
                switch (_pp.dir) {
                    case Dir::UP:
                        _data.inc();
                        _programBuilder.setInstruction(true);
                        _programBuilder.incAmount();
                        break;
                    case Dir::DOWN:
                        _data.dec();
                        _programBuilder.setInstruction(true);
                        _programBuilder.decAmount();
                        break;
                    case Dir::RIGHT:
                        if (! _data.shr()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _programBuilder.setInstruction(false);
                        _programBuilder.incAmount();
                        break;
                    case Dir::LEFT:
                        if (! _data.shl()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _programBuilder.setInstruction(false);
                        _programBuilder.decAmount();
                        break;
                }
                break;
            case Ins::TURN:
                if (_data.val() == 0) {
                    _pp.dir = (Dir)(((int)_pp.dir + 3) % 4);
                } else {
                    _pp.dir = (Dir)(((int)_pp.dir + 1) % 4);
                }
                if (_programBuilder.isInstructionSet()) {
                    _block = _programBuilder.finalizeBlock(_pp.p);

                    // Check if it is possible to exit
                    if (
                        _block != nullptr &&
                        !_settings.disableNoExitHangDetection &&
                        !_exitFinder.canExitFrom(_block)
                    ) {
                        _tracker->reportDetectedHang(HangType::NO_EXIT,
                                                     _settings.testHangDetection);
                        if (_settings.testHangDetection) {
                            fastExecution();
                        }
                        goto backtrack;
                    }

                    _block = _programBuilder.enterBlock(
                        _pp.p,
                        _data.val()==0 ? TurnDirection::COUNTERCLOCKWISE : TurnDirection::CLOCKWISE
                    );

                    if (_block != nullptr) {
                        _pp = executeCompiledBlocks();
                        if (_pp.p.col == -1) {
                            goto backtrack;
                        }
                    }
                }
                goto processInstruction;
        }

        if (_programBuilder.incSteps() > 64) {
            // Never test hang detection for this trivial hang
            _tracker->reportDetectedHang(HangType::NO_DATA_LOOP, false);
            goto backtrack;
        }

        _pp.p = insP;
        _numSteps++;
    }
backtrack:
    _data.undo(initialDataUndoP);
    _numSteps = initialSteps;
    _programBuilder.pop();
}

void ExhaustiveSearcher::initSearch() {
    _pp = _program.getStartProgramPointer();
    _numSteps = 0;
    _data.reset();
    _hangDetectionEnd = _settings.maxHangDetectionSteps;
}


Ins noResumeStack[] = { Ins::UNSET };
void ExhaustiveSearcher::search() {
    _resumeFrom = noResumeStack;
    initSearch();

    run(0);
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;
    initSearch();

    // When searching the entire tree, undo should be enabled from the start, so that search can
    // continue outside the initial subtree specified by the resumeFrom stack. For other search
    // modes it will only be enabled once this subtree is entered.
    _data.setEnableUndo(_searchMode == SearchMode::FULL_TREE);

    std::cout << "Resuming from: ";
    ::dumpInstructionStack(_resumeFrom);

    run(0);
}

void ExhaustiveSearcher::searchSubTree(Ins* resumeFrom, bool delayHangDetection) {
    _searchMode = SearchMode::SUB_TREE;
    _delayHangDetection = delayHangDetection;
    search(resumeFrom);
    _delayHangDetection = false;
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne() {
    _searchMode = SearchMode::FIND_ONE;
    search();
    _searchMode = SearchMode::FULL_TREE;
}

void ExhaustiveSearcher::findOne(Ins* resumeFrom) {
    _searchMode = SearchMode::FIND_ONE;
    search(resumeFrom);
    _searchMode = SearchMode::FULL_TREE;
}
