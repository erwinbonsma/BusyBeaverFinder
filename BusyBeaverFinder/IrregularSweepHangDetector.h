//
//  IrregularSweepHangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 06/12/2020.
//  Copyright © 2020 Erwin. All rights reserved.
//

#ifndef IrregularSweepHangDetector_h
#define IrregularSweepHangDetector_h

#include "SweepHangDetector.h"

class IrregularSweepTransitionGroup : public SweepTransitionGroup {

    bool exhibitsAperiodicGrowth();
    bool determineZeroExitSweepEndType() override;

public:
    bool determineSweepEndType() override;

};

class IrregularSweepHangDetector : public SweepHangDetector {

protected:
    bool analyzeHangBehaviour() override;
    bool analyzeTransitions() override;

public:
    IrregularSweepHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() const override { return HangType::IRREGULAR_SWEEP; }
};

#endif /* IrregularSweepHangDetector_h */
