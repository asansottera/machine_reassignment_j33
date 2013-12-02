#include "deep_shake_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>

#define TRACE_DEEP_SHAKE_ROUTINE

using namespace R12;

Move DeepShakeRoutine::randomMove(const SolutionInfo & x) {
	ProcessID p;
	MachineID src;
	MachineID dst;
	do {
		p = m_pDist(rng());
		src = x.solution()[p];
		dst = m_mDist(rng());
	} while (src == dst);
	Move move(p, src, dst);
	return move;
}

Exchange DeepShakeRoutine::randomExchange(const SolutionInfo & x) {
	ProcessID p1, p2;
	do {
		p1 = m_pDist(rng());
		p2 = m_pDist(rng());
	} while (p1 == p2);
	MachineID m1 = x.solution()[p1];
	MachineID m2 = x.solution()[p2];
	Exchange exchange(m1, p1, m2, p2);
	return exchange;
}

void DeepShakeRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", 1000);
	m_samples = parameters.param<uint64_t>("samples", 100);
}

void DeepShakeRoutine::shake(SolutionInfo & xStart, const uint64_t k) {
	uint64_t startObj = xStart.objective();
	#ifdef TRACE_DEEP_SHAKE_ROUTINE
	std::cout << "Shake " << k << " starts from objective: " << startObj << std::endl;
	#endif
	boost::uniform_int<int> methodDist(0, 1);
	uint64_t bestObj = std::numeric_limits<uint64_t>::max();
	std::vector<MachineID> bestSolution;
	for (uint64_t sample = 0; sample < m_samples; ++sample) {
		SolutionInfo x(xStart);
		MoveVerifier mv(x);
		ExchangeVerifier ev(x);
		uint64_t completed = 0;
		for (uint64_t i = 0; i < k; ++i) {
			bool found = false;
			// try to make move i
			uint64_t trials = 0;
			while (trials < m_maxTrials && !interrupted() && !found) {
				++trials;
				int method = methodDist(rng());
				if (method == 0) {
					Move move = randomMove(x);
					bool feasible = mv.feasible(move);
					++m_moveFeasibleEvalCount;
					if (feasible) {
						mv.objective(move);
						++m_moveObjectiveEvalCount;
						mv.commit(move);
						++m_moveCommitCount;
						found = true;
					}
				} else if (method == 1) {
					Exchange exchange = randomExchange(x);
					bool feasible = ev.feasible(exchange);
					++m_exchangeFeasibleEvalCount;
					if (feasible) {
						ev.objective(exchange);
						++m_exchangeObjectiveEvalCount;
						ev.commit(exchange);
						++m_exchangeCommitCount;
						found = true;
					}
				} else {
					CHECK(false);
				}
			}
			// end of move i
			if (!found) {
				completed = i;
				break;
			}	
		}
		uint64_t obj = x.objective();
		if (obj < bestObj) {
			bestObj = obj;
			bestSolution = x.solution();
		}
	}
	xStart = SolutionInfo(instance(), initial(), bestSolution);
	#ifdef TRACE_DEEP_SHAKE_ROUTINE
	int64_t diff = static_cast<int64_t>(bestObj) - static_cast<int64_t>(startObj);
	std::cout << "Shake " << k << ": accepting best of " << m_samples;
	std::cout << " samples, objective: " << xStart.objective();
	std::cout << " (delta = " << diff << ")" << std::endl;
	#endif
}
