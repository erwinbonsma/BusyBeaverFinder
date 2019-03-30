//
//  ExhaustiveSearcher.h
//  BusyBeaverFinder
//
//  Created by Erwin on 29/01/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef ExhaustiveSearcher_h
#define ExhaustiveSearcher_h

#include <stdio.h>

#include "Data.h"
#include "Program.h"

#include "CompiledProgram.h"

#include "CycleDetector.h"
#include "DataTracker.h"
#include "ProgressTracker.h"

#include "ExitFinder.h"
#include "PeriodicHangDetector.h"
#include "RegularSweepHangDetector.h"

enum class SearchMode : char {
    FULL_TREE = 0,
    SUB_TREE = 1,
    FIND_ONE = 2,
};

struct SearchSettings {
    int initialHangSamplePeriod;
    int maxSteps;
    int maxPeriodicHangDetectAttempts;
    int maxRegularSweepHangDetectAttempts;
    int maxRegularSweepExtensionCount;
    bool testHangDetection;
    bool disableNoExitHangDetection;
};

class ExhaustiveSearcher {
    SearchSettings _settings;
    // Derived settings
    int _initialHangSampleMask;

    Program _program;
    Data _data;
    bool _cycleDetectorEnabled;
    CycleDetector _cycleDetector;
    DataTracker _dataTracker;

    // Determines when to abort the search
    SearchMode _searchMode;

    // Pointer to array that can be used to resume a previous search. The last operation must be
    // UNSET.
    Ins* _resumeFrom;

    ProgramPointer _pp;
    int _numSteps;

    // Stack of instructions built up by the exhaustive search
    Ins* _instructionStack;

    // A "compiled" representation of the program
    CompiledProgram _compiledProgram;

    int _numHangDetectAttempts;

    HangDetector* _activeHangCheck;
    PeriodicHangDetector* _periodicHangDetector;
    RegularSweepHangDetector* _regularSweepHangDetector;
    ExitFinder _exitFinder;

    ProgressTracker* _tracker;

    void dumpInstructionStack(Ins* stack);
    void initInstructionStack(int size);

    void initiateNewHangCheck();
    bool periodicHangDetected();
    bool sweepHangDetected();
    bool isSweepDiverging();

    void reconfigure();

    void run(int depth);
    void branch(int depth);
public:
    ExhaustiveSearcher(int width, int height, int dataSize);
    ~ExhaustiveSearcher();

    SearchSettings getSettings() { return _settings; }

    // Updates settings. It changes buffer allocations as needed.
    void configure(SearchSettings settings);

    bool getHangDetectionTestMode() { return _settings.testHangDetection; }

    void setProgressTracker(ProgressTracker* tracker) { _tracker = tracker; }

    Program& getProgram() { return _program; }
    Data& getData() { return _data; }
    CompiledProgram& getCompiledProgram() { return _compiledProgram; }

    CycleDetector& getCycleDetector() { return _cycleDetector; }
    DataTracker& getDataTracker() { return _dataTracker; }

    ProgramPointer getProgramPointer() { return _pp; }
    int getNumSteps() { return _numSteps; }


    void search();
    void search(Ins* resumeFrom);

    void searchSubTree(Ins* resumeFrom);

    void findOne();
    void findOne(Ins* resumeFrom);

    void dumpInstructionStack();
    bool instructionStackEquals(Ins* reference);

    void dumpSettings();
    void dumpHangDetection();
    void dump();
};

#endif /* ExhaustiveSearcher_h */
