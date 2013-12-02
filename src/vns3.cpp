#include "vns3.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include "dynamic_advisor.h"
#include "random_local_search_routine.h"
#include "random_shake_routine.h"
#include "deep_shake_routine.h"
#include "deep_local_search_routine.h"
#include "smart_local_search_routine.h"
#include "sequential_local_search_routine.h"
#include "optimized_local_search_routine.h"
#include "load_cost_optimizer.h"
#include "balance_cost_optimizer.h"
#include <cmath>

#define TRACE_VNS3 0

#if TRACE_VNS3 > 0
#include <iostream>
#endif

using namespace std;
using namespace R12;

void VNS3::run() {
	const uint64_t syncPeriod = 10;
	// read parameters
	bool runLoadCostOpt = param<bool>("lcopt", false);
	bool runBalanceCostOpt = param<bool>("bcopt", false);
	m_kMin = param<uint64_t>("kMin", 1);
	m_kMax = param<uint64_t>("kMax", 100);
	m_kStep = param<uint64_t>("kStep", 1);
	std::string lsName = param<std::string>("ls", "random");
	std::string shakeName = param<std::string>("shake", "random");
	ParameterMap lsParameters = parameters().extractGroup("ls");
	ParameterMap shakeParameters = parameters().extractGroup("shake");
	// create routines
	std::unique_ptr<LocalSearchRoutine> ls;
	if (lsName.compare("random") == 0) {
		ls.reset(new RandomLocalSearchRoutine);
	} else if (lsName.compare("deep") == 0) {
		ls.reset(new DeepLocalSearchRoutine);
	} else if (lsName.compare("smart") == 0) {
		ls.reset(new SmartLocalSearchRoutine);
	} else if (lsName.compare("sequential") == 0) {
		ls.reset(new SequentialLocalSearchRoutine);
	} else if (lsName.compare("optimized") == 0) {
		ls.reset(new OptimizedLocalSearchRoutine);
	} else {
		signalError("Invalid local search routine");
		return;
	}
	std::unique_ptr<ShakeRoutine> shake;
	if (shakeName.compare("random") == 0) {
		shake.reset(new RandomShakeRoutine);
	} else if (shakeName.compare("deep") == 0) {
		shake.reset(new DeepShakeRoutine);
	} else {
		signalError("Invalid shake routine");
		return;
	}
	ls->init(instance(), initial(), flag(), m_rng, lsParameters);
	shake->init(instance(), initial(), flag(), m_rng, shakeParameters);
	// initialize
	m_rng.seed(seed());
	m_best.reset(new SolutionInfo(instance(), initial()));
	// run optimizer
	if (runLoadCostOpt) {
		LoadCostOptimizer lcopt;
		lcopt.optimize(*m_best);
	}
	if (runBalanceCostOpt) {
		BalanceCostOptimizer bcopt;
		bcopt.optimize(*m_best);
	}
	// prepare VNS
	uint64_t k = m_kMin;
	uint64_t it = 0;
	uint64_t improvements = 0;
	// run VNS iterations
	while (!interrupted()) {
		++it;
		// if time to sync, update best
		if (it % syncPeriod == 0) {
			SolutionPool::Entry entry;
			if (pool().best(entry)) {
				if (entry.obj() < m_best->objective()) {
					m_best.reset(new SolutionInfo(instance(), initial(), *(entry.ptr())));
					k = m_kMin;
				}
			}
		}
		// clone best
		m_current.reset(new SolutionInfo(*m_best));
		// find random solution in k neighborhood
		shake->shake(*m_current, k);
		// perform local search
		ls->search(*m_current);
		// check improvement
		if (m_current->objective() < m_best->objective()) {
			#if TRACE_VNS3 >= 1
			std::cout << "VNS - Improvement found @ iteration ";
			std::cout << it << ": ";
			std::cout << m_current->objective() << std::endl;
			#endif
			m_best = std::move(m_current);
			pool().push(m_best->objective(), m_best->solution());
			k = m_kMin;
			++improvements;
		} else {
			if (k == m_kMax) {
				k = m_kMin;
			} else {
				k = std::min(m_kMax, k + m_kStep);
			}
		}
	}
	#if TRACE_VNS3 >= 1
	std::cout << "Iterations: " << it << std::endl;
	std::cout << "Iterations with improvement: " << improvements << std::endl;
	#endif
	signalCompletion();
}
