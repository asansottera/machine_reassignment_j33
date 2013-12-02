#include "deep_local_search_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>

//#define TRACE_DLSR

using namespace R12;

uint64_t DeepLocalSearchRoutine::defaultMaxTrials() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

Move DeepLocalSearchRoutine::randomMove(const SolutionInfo & x) {
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

Exchange DeepLocalSearchRoutine::randomExchange(const SolutionInfo & x) {
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

void DeepLocalSearchRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", defaultMaxTrials());
	m_maxSamples = parameters.param<uint64_t>("maxSamples", 1000);
}

void DeepLocalSearchRoutine::search(SolutionInfo & x) {
	MoveVerifier mv(x);
	ExchangeVerifier ev(x);
	MethodDist methodDist(0, 1);
	uint64_t it = 0;
	uint64_t xObj = x.objective();
	uint8_t bestMethod = 0;
	Move bestMove(0, 0, 0);
	Exchange bestExchange(0, 0, 0, 0);
	uint64_t bestObj = xObj;
	while (!interrupted()) {
		++it;
		// perform one iteration
		uint64_t trials = 0;
		uint64_t samples = 0;
		while (trials < m_maxTrials && samples < m_maxSamples) {
			++trials;
			uint8_t method = methodDist(rng());
			if (method == 0) {
				Move move = randomMove(x);
				bool feasible = mv.feasible(move);
				++m_moveFeasibleEvalCount;
				if (feasible) {
					uint64_t obj = mv.objective(move);
					++m_moveObjectiveEvalCount;
					if (obj < xObj) {
						++samples;
						if (obj < bestObj) {
							// this is now the best sample
							bestMethod = 0;
							bestMove = move;
							bestObj = obj;
						}
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
						++samples;
						if (obj < bestObj) {
							// this is now the best sample
							bestMethod = 1;
							bestExchange = exchange;
							bestObj = obj;
						}
					}
				}
			} else {
				CHECK(false);
			}
		}
		// iteration completed
		// check if stuck in local minimum
		if (samples == 0 && trials == m_maxTrials) {
			#ifdef TRACE_DLSR
			std::cout << "Iteration " << it << " failed, exiting local search" << std::endl;
			#endif
			break;
		}
		// check if iteration completed
		if (samples == m_maxSamples || trials == m_maxTrials) {
			#ifdef TRACE_DLSR
			std::cout << "Iteration " << it;
			std::cout << ": collected " << samples << " samples";
			std::cout << " in " << trials << " trials.";
			std::cout << " Best sample objective: " << bestObj << std::endl;
			#endif
			// perform the best move or the best exchange
			if (bestMethod == 0) {
				mv.objective(bestMove);
				mv.commit(bestMove);
				++m_moveCommitCount;
			} else if (bestMethod == 1) {
				ev.objective(bestExchange);
				ev.commit(bestExchange);
				++m_exchangeCommitCount;
			} else {
				CHECK(false);
			}
			// update objective of current solution
			xObj = bestObj;
		}
	}
}
