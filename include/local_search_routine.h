#ifndef R12_LOCAL_SEARCH_ROUTINE_H
#define R12_LOCAL_SEARCH_ROUTINE_H

#include "heuristic_routine.h"
#include "solution_info.h"

namespace R12 {

class LocalSearchRoutine : public HeuristicRoutine {
public:
	virtual void search(SolutionInfo & x) = 0;
};

}

#endif
