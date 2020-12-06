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

class IrregularSweepHangDetector : public SweepHangDetector {

protected:
    bool shouldCheckNow(bool loopContinues) override;

    bool analyzeHangBehaviour() override;
    bool analyzeTransitions() override;

public:
    IrregularSweepHangDetector(const ProgramExecutor& executor);

    virtual HangType hangType() const override { return HangType::IRREGULAR_SWEEP; }
};

#endif /* IrregularSweepHangDetector_h */
