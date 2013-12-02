#include "random_shake_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>

using namespace R12;

Move RandomShakeRoutine::randomMove(const SolutionInfo & x) {
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

Exchange RandomShakeRoutine::randomExchange(const SolutionInfo & x) {
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

void RandomShakeRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", 1000);
}

void RandomShakeRoutine::shake(SolutionInfo & x, const uint64_t k) {
	MoveVerifier mv(x);
	ExchangeVerifier ev(x);
	boost::uniform_int<int> methodDist(0, 1);
	bool stopped = false;
	for (uint64_t i = 0; i < k; ++i) {
		bool found = false;
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
		if (!found) {
			#ifdef TRACE_RANDOM_SHAKE_ROUTINE
			std::cout << "Shake failed after ";
			std::cout << i << "/" << k << " moves, objective = ";
			std::cout << x.objective() << std::endl;
			#endif
			stopped = true;
			break;
		}
	}
	if (!stopped) {
		#ifdef TRACE_RANDOM_SHAKE_ROUTINE
		std::cout << "Shake completed " << k << " moves, objective = ";
		std::cout << x.objective() << std::endl;
		#endif
	}

}
