#ifndef R12_SHAKE_ROUTINE_H
#define R12_SHAKE_ROUTINE_H

#include "heuristic_routine.h"
#include "solution_info.h"
#include <boost/cstdint.hpp>

namespace R12 {

class ShakeRoutine : public HeuristicRoutine {
public:
	virtual void shake(SolutionInfo & x, const uint64_t k) = 0;
};

}

#endif
