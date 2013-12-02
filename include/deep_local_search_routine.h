#ifndef R12_DEEP_LOCAL_SEARCH_ROUTINE_H
#define R12_DEEP_LOCAL_SEARCH_ROUTINE_H

#include "common.h"
#include "problem.h"
#include "solution_info.h"
#include "move.h"
#include "exchange.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include "local_search_routine.h"
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class DeepLocalSearchRoutine : public LocalSearchRoutine {
private:
	typedef boost::random::uniform_smallint<uint8_t> MethodDist;
	typedef boost::random::uniform_smallint<ProcessID> ProcessDist;
	typedef boost::random::uniform_smallint<MachineID> MachineDist;
private: // constructed during initialization
	ProcessDist m_pDist;
	MachineDist m_mDist;
private: // parameters
	uint64_t m_maxTrials;
	uint64_t m_maxSamples;
private: // statistics
	uint64_t m_moveFeasibleEvalCount;
	uint64_t m_moveObjectiveEvalCount;
	uint64_t m_moveCommitCount;
	uint64_t m_exchangeFeasibleEvalCount;
	uint64_t m_exchangeObjectiveEvalCount;
	uint64_t m_exchangeCommitCount;
private:
	uint64_t defaultMaxTrials();
	Move randomMove(const SolutionInfo & x);
	Exchange randomExchange(const SolutionInfo & x);
public:
	virtual void configure(const ParameterMap & parameters);
	virtual void search(SolutionInfo & x);
};

}

#endif
