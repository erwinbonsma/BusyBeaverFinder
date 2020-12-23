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

ExhaustiveSearcher::ExhaustiveSearcher(int width, int height, int dataSize) :
    _program(width, height),
    _data(dataSize),
    _runSummary(),
    _exitFinder(_program, _interpretedProgramBuilder),
    _fastExecutor(dataSize)
{
    initInstructionStack(width * height);

    _hangDetectors[0] = new PeriodicHangDetector(*this);
    _hangDetectors[1] = new MetaPeriodicHangDetector(*this);
    _hangDetectors[2] = new GliderHangDetector(*this);
    _hangDetectors[3] = new PeriodicSweepHangDetector(*this);
    _hangDetectors[4] = new IrregularSweepHangDetector(*this);

    _zArrayHelperBuf = nullptr;

    // Set default search mode
    _searchMode = SearchMode::FULL_TREE;
    _delayHangDetection = false;

    // Set default search settings
    _settings.maxSteps = 1024;
    _settings.maxHangDetectionSteps = _settings.maxSteps;
    _settings.undoCapacity = 1024;
    _settings.testHangDetection = false;
    _settings.disableNoExitHangDetection = false;
    reconfigure();
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    delete[] _instructionStack;

    for (auto hangDetector : _hangDetectors) {
        delete hangDetector;
    }

    delete[] _zArrayHelperBuf;
}

void ExhaustiveSearcher::configure(SearchSettings settings) {
    _settings = settings;
    reconfigure();
}

void ExhaustiveSearcher::reconfigure() {
    // Default. Will be adapted during search when fast resume is enabled.
    _hangDetectionEnd = _settings.maxHangDetectionSteps;

    _data.setStackSize(_settings.undoCapacity);

    int capacity = _settings.maxHangDetectionSteps;
    if (_zArrayHelperBuf == nullptr || _runSummary[1].getCapacity() != capacity) {
        // The arrays need to be re-allocated

        delete[] _zArrayHelperBuf; // okay to delete when null

        _zArrayHelperBuf = new int[capacity / 2];

        _runSummary[0].setCapacity(capacity, _zArrayHelperBuf);
        _runSummary[1].setCapacity(capacity, _zArrayHelperBuf);
    }
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
    << ", DataSize = " << _data.getSize()
    << ", MaxSteps = " << _settings.maxHangDetectionSteps << "/" << _settings.maxSteps
    << ", UndoCapacity = " << _settings.undoCapacity
    << ", TestHangDetection = " << _settings.testHangDetection
    << std::endl;
}

void ExhaustiveSearcher::dumpExecutionState() const {
    std::cout << "Num steps: " << _numSteps << std::endl;
    dumpInstructionStack();
    ProgramExecutor::dumpExecutionState();
}

void ExhaustiveSearcher::dump() {
    _program.dump(_pp.p);
    _data.dump();
}

void ExhaustiveSearcher::setProgressTracker(ProgressTracker* tracker) {
    _tracker = tracker;
    _fastExecutor.setProgressTracker(tracker);
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
    _fastExecutor.execute(_interpretedProgramBuilder.getEntryBlock(), _settings.maxSteps);
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

    _interpretedProgramBuilder.enterBlock(_block);
    return _interpretedProgramBuilder.getStartProgramPointer(_block, _program);
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocksWithHangDetection() {
    ProgramBlock* entryBlock = _interpretedProgramBuilder.getEntryBlock();

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
                    _tracker->reportDetectedHang(hangDetector, _settings.testHangDetection);
                    if (_settings.testHangDetection) {
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

    _interpretedProgramBuilder.enterBlock(_block);
    return _interpretedProgramBuilder.getStartProgramPointer(_block, _program);
}

ProgramPointer ExhaustiveSearcher::executeCompiledBlocks() {
    if (!_data.hasUndoCapacity()) {
        _fastExecutor.execute(_interpretedProgramBuilder.getEntryBlock(), _settings.maxSteps);
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

    _interpretedProgramBuilder.push();

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
                        _interpretedProgramBuilder.setInstruction(true);
                        _interpretedProgramBuilder.incAmount();
                        break;
                    case Dir::DOWN:
                        _data.dec();
                        _interpretedProgramBuilder.setInstruction(true);
                        _interpretedProgramBuilder.decAmount();
                        break;
                    case Dir::RIGHT:
                        if (! _data.shr()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _interpretedProgramBuilder.setInstruction(false);
                        _interpretedProgramBuilder.incAmount();
                        break;
                    case Dir::LEFT:
                        if (! _data.shl()) {
                            _tracker->reportError();
                            goto backtrack;
                        }
                        _interpretedProgramBuilder.setInstruction(false);
                        _interpretedProgramBuilder.decAmount();
                        break;
                }
                break;
            case Ins::TURN:
                if (_data.val() == 0) {
                    _pp.dir = (Dir)(((int)_pp.dir + 3) % 4);
                } else {
                    _pp.dir = (Dir)(((int)_pp.dir + 1) % 4);
                }
                if (_interpretedProgramBuilder.isInstructionSet()) {
                    _block = _interpretedProgramBuilder.finalizeBlock(_pp.p);

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

                    _block = _interpretedProgramBuilder.enterBlock(
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

        if (_interpretedProgramBuilder.incSteps() > 64) {
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
    _interpretedProgramBuilder.pop();
}

Ins noResumeStack[] = { Ins::UNSET };
void ExhaustiveSearcher::search() {
    _resumeFrom = noResumeStack;

    _pp.p = _program.getStartProgramPointer();
    _pp.dir = Dir::UP;
    _numSteps = 0;
    _data.reset();
    run(0);
}

void ExhaustiveSearcher::search(Ins* resumeFrom) {
    _resumeFrom = resumeFrom;

    std::cout << "Resuming from: ";
    ::dumpInstructionStack(_resumeFrom);

    _pp.p = _program.getStartProgramPointer();
    _pp.dir = Dir::UP;
    _numSteps = 0;
    _data.reset();

    // When searching the entire tree, undo should be enabled from the start, so that search can
    // continue outside the initial subtree specified by the resumeFrom stack. For other search
    // modes it will only be enabled once this subtree is entered.
    _data.setEnableUndo(_searchMode == SearchMode::FULL_TREE);

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
