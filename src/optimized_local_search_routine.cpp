#include "optimized_local_search_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>

//#define TRACE_OPTLSR

using namespace R12;

uint64_t OptimizedLocalSearchRoutine::defaultMaxTrials() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

void OptimizedLocalSearchRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", defaultMaxTrials());
	m_maxSamples = parameters.param<uint64_t>("maxSamples", 1000);
	m_block = parameters.param<uint64_t>("block", 20);
}

void OptimizedLocalSearchRoutine::search(SolutionInfo & x) {
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
		std::vector<ProcessID> pvec(m_block);
		std::vector<MachineID> mvec(m_block);
		std::vector<ProcessID> p1vec(m_block);
		std::vector<ProcessID> p2vec(m_block);
		bool renewMove = true;
		bool renewExchange = true;
		uint64_t pi = 0;
		uint64_t mi = 0;
		uint64_t p1i = 0;
		uint64_t p2i = 0;
		while (trials < m_maxTrials && samples < m_maxSamples) {
			if (renewMove) {
				for (uint64_t i = 0; i < m_block; ++i) {
					pvec[i] = m_pDist(rng());
					mvec[i] = m_mDist(rng());
				}
				renewMove = false;
			}
			if (renewExchange) {
				for (uint64_t i = 0; i < m_block; ++i) {
					p1vec[i] = m_pDist(rng());
					p2vec[i] = m_pDist(rng());
				}
				renewExchange = false;
			}
			// perform a trial
			++trials;
			uint8_t method = methodDist(rng());
			if (method == 0) {
				// select next move
				const ProcessID p = pvec[pi];
				const MachineID dst = mvec[mi];
				const MachineID src = x.solution()[p];
				Move move(p, src, dst);
				// check feasibility and objective
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
				// increase indexes
				++mi;
				if (mi == m_block) {
					mi = 0;
					++pi;
					if (pi == m_block) {
						pi = 0;
						renewMove = true;
					}
				}
			} else if (method == 1) {
				// select next exchange
				const ProcessID p1 = p1vec[p1i];
				const ProcessID p2 = p2vec[p2i];
				const MachineID m1 = x.solution()[p1];
				const MachineID m2 = x.solution()[p2];
				Exchange exchange(m1, p1, m2, p2);
				// check feasibility and objective
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
				// increase indexes
				++p2i;
				if (p2i == m_block) {
					p2i = 0;
					++p1i;
					if (p1i == m_block) {
						p1i = 0;
						renewExchange = true;
					}
				}
			} else {
				CHECK(false);
			}
		}
		// iteration completed
		// check if stuck in local minimum
		if (samples == 0 && trials == m_maxTrials) {
			#ifdef TRACE_OPTLSR
			std::cout << "Iteration " << it << " failed, exiting local search" << std::endl;
			#endif
			break;
		}
		// check if iteration completed
		if (samples == m_maxSamples || trials == m_maxTrials) {
			#ifdef TRACE_OPTLSR
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
