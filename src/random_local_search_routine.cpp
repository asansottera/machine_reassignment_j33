#include "random_local_search_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>

using namespace R12;

uint64_t RandomLocalSearchRoutine::defaultMaxTrials() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

Move RandomLocalSearchRoutine::randomMove(const SolutionInfo & x) {
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

Exchange RandomLocalSearchRoutine::randomExchange(const SolutionInfo & x) {
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

void RandomLocalSearchRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", defaultMaxTrials());
}

void RandomLocalSearchRoutine::search(SolutionInfo & x) {
	MoveVerifier mv(x);
	ExchangeVerifier ev(x);
	MethodDist methodDist(0, 1);
	uint64_t it = 0;
	uint64_t trials = 0;
	uint64_t xObj = x.objective();
	while (trials < m_maxTrials && !interrupted()) {
		++trials;
		int method = methodDist(rng());
		if (method == 0) {
			Move move = randomMove(x);
			bool feasible = mv.feasible(move);
			++m_moveFeasibleEvalCount;
			if (feasible) {
				uint64_t obj = mv.objective(move);
				++m_moveObjectiveEvalCount;
				if (obj < xObj) {
					mv.commit(move);
					xObj = obj;
					++m_moveCommitCount;
					trials = 0;
					++it;
				}
			}
		} else if (method == 1) {
			Exchange exchange = randomExchange(x);
			bool feasible = ev.feasible(exchange);
			++m_exchangeFeasibleEvalCount;
			if (feasible) {
				uint64_t obj = ev.objective(exchange);
				++m_exchangeObjectiveEvalCount;
				if (obj < xObj) {
					ev.commit(exchange);
					xObj = obj;
					++m_exchangeCommitCount;
					trials = 0;
					++it;
				}
			}
		} else {
			CHECK(false);
		}
	}
}
