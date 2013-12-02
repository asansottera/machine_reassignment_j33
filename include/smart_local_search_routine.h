#ifndef R12_SMART_LOCAL_SEARCH_ROUTINE_H
#define R12_SMART_LOCAL_SEARCH_ROUTINE_H

#include "common.h"
#include "problem.h"
#include "solution_info.h"
#include "move.h"
#include "exchange.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include "local_search_routine.h"
#include "problem_info.h"
#include <vector>
#include <memory>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class SmartLocalSearchRoutine : public LocalSearchRoutine {
private:
	typedef boost::random::uniform_smallint<uint8_t> MethodDist;
	typedef boost::random::uniform_smallint<ProcessID> ProcessDist;
	typedef boost::random::uniform_smallint<MachineID> MachineDist;
private: // constructed during initialization
	ProcessDist m_pDist;
	MachineDist m_mDist;
	std::unique_ptr<ProblemInfo> m_problemInfo;
private: // parameters
	uint64_t m_maxTrials;
	uint64_t m_maxSamples;
	bool  m_unitStep;
private:
	uint64_t defaultMaxTrials();
public:
	virtual void configure(const ParameterMap & parameters);
	virtual void search(SolutionInfo & x);
};

}

#endif
