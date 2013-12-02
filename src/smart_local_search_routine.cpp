#include "smart_local_search_routine.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include <cmath>
#include <iostream>

#define TRACE_SLSR 2

using namespace R12;

uint64_t SmartLocalSearchRoutine::defaultMaxTrials() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

void SmartLocalSearchRoutine::configure(const ParameterMap & parameters) {
	// m_problemInfo.reset(new ProblemInfo(instance(), initial()));
	// m_problemInfo->report(std::cout);
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_maxTrials = parameters.param<uint64_t>("maxTrials", defaultMaxTrials());
	m_maxSamples = parameters.param<uint64_t>("maxSamples", 1000);
	m_unitStep = parameters.param<bool>("unitStep", false);
}

void SmartLocalSearchRoutine::search(SolutionInfo & x) {
	ProcessCount pCount = instance().processes().size();
	MachineCount mCount = instance().machines().size();
	MoveVerifier mv(x);
	ExchangeVerifier ev(x);
	MethodDist methodDist(0, 1);
	uint32_t renewal = std::min<uint32_t>(50, std::min(pCount, mCount));
	ProcessDist pStepDist(1, std::max(1u, pCount / renewal));
	MachineDist mStepDist(1, std::max(1u, mCount / renewal));
	#if TRACE_SLSR >= 1
	std::cout << "Renewal period: " << renewal << std::endl;
	std::cout << "Process max step: " << pStepDist.max() << std::endl;
	std::cout << "Machine max step: " << mStepDist.max() << std::endl;
	#endif
	uint64_t it = 0;
	uint64_t xObj = x.objective();
	uint8_t bestMethod = 0;
	Move bestMove(0, 0, 0);
	Exchange bestExchange(0, 0, 0, 0);
	uint64_t bestObj = xObj;
	while (!interrupted()) {
		++it;
		// perform one iteration
		uint64_t samples = 0;
		uint64_t trials = 0;
		ProcessID p = 0;
		ProcessID pStep = 1;
		MachineID dst = 0;
		MachineID dstStep = 1;
		ProcessID p1 = 0;
		ProcessID p1Step = 1;
		ProcessID p2 = 0;
		ProcessID p2Step = 1;
		while (samples < m_maxSamples && trials < m_maxTrials) {
			// renew starting point
			if (trials % renewal == 0) {
				p = m_pDist(rng());
				pStep = m_unitStep ? 1 : pStepDist(rng());
				dst = m_mDist(rng());
				dstStep = m_unitStep ? 1 : mStepDist(rng());
				p1 = m_pDist(rng());
				p1Step = m_unitStep ? 1 : pStepDist(rng());
				p2 = m_pDist(rng());
				p2Step = m_unitStep ? 1 : pStepDist(rng());
			}
			// increase trial count
			++trials;
			// try move or exchange
			if (trials % 2 == 0) {
				p = (p + pStep) % pCount;
				dst = (dst + dstStep) % mCount;
				MachineID src = x.solution()[p];
				if (src != dst) {
					Move move(p, src, dst);
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
			} else if (trials % 2 == 1) {
				p1 = (p1 + p1Step) % pCount;
				p2 = (p2 + p2Step) % pCount;
				MachineID m1 = x.solution()[p1];
				MachineID m2 = x.solution()[p2];
				if (p1 != p2 && m1 != m2) {
					Exchange exchange(m1, p1, m2, p2);
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
			}
		}
		// iteration completed
		// check if stuck in local minimum
		if (samples == 0 && trials == m_maxTrials) {
			#if TRACE_SLSR >= 2
			std::cout << "Iteration " << it << " failed, exiting local search" << std::endl;
			#endif
			break;
		}
		// check if iteration completed
		if (samples == m_maxSamples || trials == m_maxTrials) {
			#if TRACE_SLSR >= 2
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
