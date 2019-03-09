//
//  HangDetector.h
//  BusyBeaverFinder
//
//  Created by Erwin on 09/03/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef HangDetector_h
#define HangDetector_h

enum class HangDetectionResult : char {
    // Detection is still ongoing. It's too early to conclude
    ONGOING = 0,

    // A hang was detected
    HANGING = 1,

    // Failed to detect a hang
    FAILED = 2
};

class HangDetector {
public:
    virtual ~HangDetector() {}

    virtual void start() = 0;
    virtual HangDetectionResult detectHang() = 0;
};

#endif /* HangDetector_h */
