#ifndef R12_DEEP_SHAKE_ROUTINE_H
#define R12_DEEP_SHAKE_ROUTINE_H

#include "common.h"
#include "problem.h"
#include "solution_info.h"
#include "move.h"
#include "exchange.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include "shake_routine.h"
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class DeepShakeRoutine : public ShakeRoutine {
private:
	typedef boost::random::uniform_smallint<uint8_t> MethodDist;
	typedef boost::random::uniform_smallint<ProcessID> ProcessDist;
	typedef boost::random::uniform_smallint<MachineID> MachineDist;
private: // constructed during initialization
	ProcessDist m_pDist;
	MachineDist m_mDist;
private: // parameters
	uint64_t m_samples;
	uint64_t m_maxTrials;
private: // statistics
	uint64_t m_moveFeasibleEvalCount;
	uint64_t m_moveObjectiveEvalCount;
	uint64_t m_moveCommitCount;
	uint64_t m_exchangeFeasibleEvalCount;
	uint64_t m_exchangeObjectiveEvalCount;
	uint64_t m_exchangeCommitCount;
private:
	Move randomMove(const SolutionInfo & x);
	Exchange randomExchange(const SolutionInfo & x);
public:
	virtual void configure(const ParameterMap & parameters);
	virtual void shake(SolutionInfo & x, const uint64_t k);
};

}

#endif
