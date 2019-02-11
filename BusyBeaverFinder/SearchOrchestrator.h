//
//  SearchOrchestrator.h
//  BusyBeaverFinder
//
//  Created by Erwin on 11/02/19.
//  Copyright © 2019 Erwin Bonsma.
//

#ifndef SearchOrchestrator_h
#define SearchOrchestrator_h

#include <stdio.h>

class ExhaustiveSearcher;

class SearchOrchestrator {
    ExhaustiveSearcher& _searcher;

public:
    SearchOrchestrator(ExhaustiveSearcher& searcher);

    void search();
};

#endif /* SearchOrchestrator_h */
