#include "sequential_local_search_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>
#include <iostream>

#define TRACE_SEQLSR 0

using namespace R12;

uint64_t SequentialLocalSearchRoutine::defaultMaxTrials() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

void SequentialLocalSearchRoutine::configure(const ParameterMap & parameters) {
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", defaultMaxTrials());
	m_maxSamples = parameters.param<uint64_t>("maxSamples", 1000);
}

void SequentialLocalSearchRoutine::search(SolutionInfo & x) {
	ProcessCount pCount = instance().processes().size();
	MachineCount mCount = instance().machines().size();
	MoveVerifier mv(x);
	ExchangeVerifier ev(x);
	MethodDist methodDist(0, 1);
	// trial count
	MachineCount mSequence = std::min<MachineCount>(30, mCount);
	ProcessCount pSequence = std::min<ProcessCount>(30, pCount);
	#if TRACE_SEQLSR >= 1
	std::cout << "SEQLSR: max trials = " << m_maxTrials << std::endl;
	std::cout << "SEQLSR: max samples = " << m_maxSamples << std::endl;
	std::cout << "SEQLSR: process sequence = " << pSequence << std::endl;
	std::cout << "SEQLSR: machine sequence = " << mSequence << std::endl;
	#endif
	// local search state
	uint64_t it = 0;
	uint64_t xObj = x.objective();
	uint8_t bestMethod = 0;
	Move bestMove(0, 0, 0);
	Exchange bestExchange(0, 0, 0, 0);
	uint64_t bestObj = xObj;
	// run local search iterations
	while (!interrupted()) {
		++it;
		// iteration data
		uint64_t samples = 0;
		uint64_t trials = 0;
		// move iteration data
		ProcessID pStart = m_pDist(rng());
		ProcessCount i = 0;
		MachineID mStart = m_mDist(rng());
		MachineCount j = 0;
		// init exchange
		MachineID m1 = m_mDist(rng());
		MachineID m2 = m_mDist(rng());
		std::vector<ProcessID> m1procs;
		std::vector<ProcessID> m2procs;
		m1procs.reserve(pCount/mCount);
		m2procs.reserve(pCount/mCount);
		ProcessCount k1 = 0;
		ProcessCount k2 = 0;
		bool initProcs = true;
		while (samples < m_maxSamples && trials < m_maxTrials) {
			if (trials % 2 == 0) {
				// try move
				ProcessID p = (pStart + i) % pCount;
				MachineID m = (mStart + j) % mCount;
				MachineID src = x.solution()[p];
				Move move(p, src, m);
				if (m != src) {
					++trials;
					if (mv.feasible(move)) {
						uint64_t obj = mv.objective(move);
						if (obj < xObj) {
							++samples;
							if (obj < bestObj) {
								bestObj = obj;
								bestMove = move;
								bestMethod = 0;
							}
						}
					}
				}
				// increase inner loop counter
				++j;
				if (j == mSequence) {
					j = 0;
					// increase outer loop counter
					++i;
					if (i == pSequence) {
						i = 0;
						// reset
						pStart = m_pDist(rng());
						mStart = m_mDist(rng());
					}
				}
			} else {
				// initialize at first iteration or after a reset
				if (initProcs) {
					for (ProcessID h = 0; h < pCount; ++h) {
						MachineID m = x.solution()[h];
						if (m == m1) {
							m1procs.push_back(h);
						} else if (m == m2) {
							m2procs.push_back(h);
						}
					}
					initProcs = false;
				}
				// always true unless one of the two sets is empty
				if (k1 < m1procs.size() && k2 < m2procs.size()) {
					// try exchange
					ProcessID p1 = m1procs[k1];
					ProcessID p2 = m2procs[k2];
					Exchange exchange(m1, p1, m2, p2);
					++trials;
					if (ev.feasible(exchange)) {
						uint64_t obj = ev.objective(exchange);
						if (obj < xObj) {
							++samples;
							if (obj < bestObj) {
								bestObj = obj;
								bestExchange = exchange;
								bestMethod = 1;
							}
						}
					}
				}
				// increase inner iteration counter
				++k2;
				if (k2 == m2procs.size()) {
					k2 = 0;
					// increase outer iteration counter
					++k1;
					if (k1 == m1procs.size()) {
						// reset
						m1procs.clear();
						m2procs.clear();
						m1 = m_mDist(rng());
						m2 = m_mDist(rng());
						k1 = 0;
						k2 = 0;
						initProcs = true;						
					}
				}
			}
		}
		// iteration completed
		// check if stuck in local minimum
		if (samples == 0 && trials >= m_maxTrials) {
			#if TRACE_SEQLSR >= 2
			std::cout << "Iteration " << it << " failed, exiting local search" << std::endl;
			#endif
			break;
		}
		// check if iteration completed
		if (samples >= m_maxSamples || trials >= m_maxTrials) {
			#if TRACE_SEQLSR >= 2
			std::cout << "Iteration " << it;
			std::cout << ": collected " << samples << " samples";
			std::cout << " in " << trials << " trials.";
			std::cout << " Best sample objective: " << bestObj << std::endl;
			#endif
			// perform the best move or the best exchange
			if (bestMethod == 0) {
				mv.objective(bestMove);
				mv.commit(bestMove);
			} else if (bestMethod == 1) {
				ev.objective(bestExchange);
				ev.commit(bestExchange);
			} else {
				CHECK(false);
			}
			// update objective of current solution
			xObj = bestObj;
		}
	}
}
