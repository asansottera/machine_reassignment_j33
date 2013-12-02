#include "vns.h"
#include "solution_info.h"
#include "fast_local_search.h"
#include "best_improvement_local_search.h"
#include "batch_verifier.h"
#include "smart_shaker.h"
#include "move_verifier.h"
#include <vector>
#include <stdexcept>
#include <boost/random.hpp>

#ifdef __INTEL_COMPILER
#pragma warning(disable:2259)
#endif

// #define CHECK_VNS
#define TRACE_VNS

#ifdef TRACE_VNS
#include <iostream>
#endif

#ifdef CHECK_VNS
#include "verifier.h"
#endif

using namespace R12;

void VNS::localSearch() {
	boost::random::uniform_int_distribution<ProcessID> pDist(0, instance().processes().size() - 1);
	boost::random::uniform_int_distribution<MachineID> mDist(0, instance().machines().size() - 1);
	MoveVerifier mv(*m_current);
	uint64_t it =0;
	while (!interrupted()) {
		++it;
		bool found = false;
		Move bestMove(0, 0, 0);
		uint64_t bestObj = m_current->objective();
		uint64_t trials = 0;
		uint64_t samples = 0;
		while (!interrupted() && trials < m_maxTrialsLS && samples < m_maxSamplesLS) {
			++trials;
			ProcessID p;
			MachineID src;
			MachineID dst;
			do {
				p = pDist(m_rng);
				src = m_current->solution()[p];
				dst = mDist(m_rng);
			} while (src == dst);
			Move move(p, src, dst);
			bool feasible = mv.feasible(move);
			if (feasible) {
				uint64_t objective = mv.objective(move);
				++samples;
				if (objective < bestObj) {
					bestMove = move;
					bestObj = objective;
					found = true;
				}
			}
		}
		if (found) {
			mv.objective(bestMove);
			mv.commit(bestMove);
		} else {
			break;
		}
	}
}


uint64_t VNS::defaultMaxTrialsLS() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * std::log10(mCount));
}

void VNS::run() {
	const ProcessCount pCount = instance().processes().size();
	// read configuration
	const uint32_t kMin = hasParam("kMin") ? param<uint32_t>("kMin") : 2;
	const uint32_t kMax = hasParam("kMax") ? param<uint32_t>("kMax") : 5;
	const uint32_t trials = hasParam("trials") ? param<uint32_t>("trials") : 5;
	const uint32_t maxInf = hasParam("maxInf") ? param<uint32_t>("maxInf") : 3;
	m_maxTrialsLS = param<uint64_t>("maxTrialsLS", defaultMaxTrialsLS());
	m_maxSamplesLS = param<uint64_t>("maxSamplesLS", pCount);
	// initialize RNG
	m_rng.seed(seed());
	// initialize shaker
	SmartShaker shaker(trials, maxInf);
	shaker.init(instance(), initial(), seed(), flag());
	// initialize solution info
	m_current.reset(new SolutionInfo(instance(), initial()));
	// run local search to get starting solution
	localSearch();
	// set best and publish to pool
	m_best = std::move(m_current);
	pool().push(m_best->objective(), m_best->solution());
	// start VNS
	uint64_t it = 0;
	uint32_t k = kMin;
	uint64_t syncPeriod = 100;
	uint64_t diversifyPeriod = 100;
	uint64_t concentratePeriod = 100;
	#ifdef CHECK_VNS
	Verifier v;
	#endif
	while (!interrupted()) {
		++it;
		// periodically sync with best result
		if (it % syncPeriod == 0) {
			SolutionPool::Entry entry;
			if (pool().best(entry)) {
				if (entry.obj() < m_best->objective()) {
					m_best.reset(new SolutionInfo(instance(), initial(), *(entry.ptr())));
					k = kMin;
				}
			}
		}
		// periodically start from a good solution different from the best known one
		if (it % diversifyPeriod == 25) {
			SolutionPool::Entry hdEntry;
			if (pool().randomHighDiversity(hdEntry)) {
				m_current.reset(new SolutionInfo(instance(), initial(), *hdEntry.ptr()));
			}
		} else if (it % concentratePeriod == 75) {
			SolutionPool::Entry hqEntry;
			if (pool().randomHighQuality(hqEntry)) {
				m_current.reset(new SolutionInfo(instance(), initial(), *hqEntry.ptr()));
			}
		} else {
			// most of the times start from the best known solution
			m_current.reset(new SolutionInfo(*m_best));
		}
		// make random jump
		shaker.shake(k, *m_current);
		// since shaking might take a long time, check for interruption
		if (interrupted()) {
			#ifdef TRACE_VNS
			std::cout << "VNS - Interrupted during shake @ iteration " << it << std::endl;
			#endif
			break;
		}
		#ifdef CHECK_VNS
		//  verify shaking led to a feasible solution
		if (!v.verify(instance(), initial(), m_current->solution()).feasible()) {
			throw std::runtime_error("VNS - Shaking lead to an unfeasible solution");
		}
		#endif
		// do a local search
		localSearch();
		#ifdef CHECK_VNS
		//  verify shaking led to a feasible solution
		if (!v.verify(instance(), initial(), m_current->solution()).feasible()) {
			throw std::runtime_error("VNS - Local search lead to an unfeasible solution");
		}
		#endif
		// update best solution if improved if needed
		if (m_current->objective() < m_best->objective()) {
			m_best = std::move(m_current);
			#ifdef TRACE_VNS
			std::cout << "VNS - Improvement found @ iteration " << it << ": " << m_best->objective() << std::endl;
			#endif
			pool().push(m_best->objective(), m_best->solution());
			// restart from small neighborhood
			k = kMin;
		} else {
			// move to another neighborhood
			k = kMin + (k - kMin + 1) % (kMax - kMin + 1);
		}
	}
	#ifdef TRACE_VNS
	std::cout << "VNS - Completed after " << it << " iterations" << std::endl;
	std::cout << "VNS - Failures per shake " << shaker.failuresPerShake() << std::endl;
	#endif
	signalCompletion();
}

