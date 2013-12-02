#include "vns2.h"

#include "move_verifier.h"
#include "exchange_verifier.h"
#include "dynamic_advisor.h"
#include <cmath>

#define TRACE_VNS2 1

#if TRACE_VNS2 > 0
#include <iostream>
#endif

using namespace std;
using namespace R12;

Move VNS2::randomMove() {
	if (!m_useAdvisor) {
		ProcessID p;
		MachineID src;
		MachineID dst;
		do {
			p = m_pDist(m_rng);
			src = m_current->solution()[p];
			dst = m_mDist(m_rng);
		} while (src == dst);
		Move move(p, src, dst);
		return move;
	} else {
		return m_advisor->adviseMove(*m_current);
	}
}

Exchange VNS2::randomExchange() {
	if (!m_useAdvisor) {
		ProcessID p1, p2;
		do {
			p1 = m_pDist(m_rng);
			p2 = m_pDist(m_rng);
		} while (p1 == p2);
		MachineID m1 = m_current->solution()[p1];
		MachineID m2 = m_current->solution()[p2];
		Exchange exchange(m1, p1, m2, p2);
		return exchange;
	} else {
		return m_advisor->adviseExchange(*m_current);
	}
}

void VNS2::localSearch() {
	MoveVerifier mv(*m_current);
	ExchangeVerifier ev(*m_current);
	std::unique_ptr<DynamicAdvisor> adv;
	if (m_useDynamicAdvisor) {
		adv.reset(new DynamicAdvisor(*m_current, seed()));
	}
	MethodDist methodDist(0, 1);
	uint64_t it = 0;
	uint64_t trials = 0;
	while (trials < m_maxTrialsLS && !interrupted()) {
		++trials;
		int method = methodDist(m_rng);
		if (method == 0) {
			Move move = m_useDynamicAdvisor ? adv->adviseMove() : randomMove();
			bool feasible = mv.feasible(move);
			++m_moveFeasibleEvalCount;
			++m_moveEvalLocalSearch;
			if (feasible) {
				uint64_t obj = mv.objective(move);
				++m_moveObjectiveEvalCount;
				if (obj < m_current->objective()) {
					mv.commit(move);
					if (m_useDynamicAdvisor) {
						adv->notify(move);
					}
					++m_moveCommitCount;
					++m_moveCommitLocalSearch;
					trials = 0;
					++it;
				}
			}
		} else if (method == 1) {
			Exchange exchange = randomExchange();
			bool feasible = ev.feasible(exchange);
			++m_exchangeFeasibleEvalCount;
			++m_exchangeEvalLocalSearch;
			if (feasible) {
				uint64_t obj = ev.objective(exchange);
				++m_exchangeObjectiveEvalCount;
				if (obj < m_current->objective()) {
					ev.commit(exchange);
					if (m_useDynamicAdvisor) {
						adv->notify(exchange);
					}
					++m_exchangeCommitCount;
					++m_exchangeCommitLocalSearch;
					trials = 0;
					++it;
				}
			}
		} else {
			CHECK(false);
		}
	}
	#if TRACE_VNS2 >= 2
	std::cout << "VNS - Local search iterations: " << it;
	std::cout << ", objective = " << m_current->objective() << std::endl;
	#endif
}

void VNS2::shake(uint64_t k) {
	MoveVerifier mv(*m_current);
	ExchangeVerifier ev(*m_current);
	boost::uniform_int<int> methodDist(0, 1);
	bool stopped = false;
	for (uint64_t i = 0; i < k; ++i) {
		bool found = false;
		uint64_t trials = 0;
		while (trials < m_maxTrialsShake && !interrupted() && !found) {
			++trials;
			int method = methodDist(m_rng);
			if (method == 0) {
				Move move = randomMove();
				bool feasible = mv.feasible(move);
				++m_moveFeasibleEvalCount;
				++m_moveEvalShake;
				if (feasible) {
					mv.objective(move);
					++m_moveObjectiveEvalCount;
					mv.commit(move);
					++m_moveCommitCount;
					++m_moveCommitShake;
					#if TRACE_VNS2 >= 3
					std::cout << "VNS - Shake: accepted move" << std::endl;
					#endif
					found = true;
				} else {
					#if TRACE_VNS2 >= 3
					std::cout << "VNS - Shake: rejected move" << std::endl;
					#endif
				}
			} else if (method == 1) {
				Exchange exchange = randomExchange();
				bool feasible = ev.feasible(exchange);
				++m_exchangeFeasibleEvalCount;
				++m_exchangeEvalShake;
				if (feasible) {
					ev.objective(exchange);
					++m_exchangeObjectiveEvalCount;
					ev.commit(exchange);
					++m_exchangeCommitCount;
					++m_exchangeCommitShake;
					#if TRACE_VNS2 >= 3
					std::cout << "VNS - Shake: accepted exchange" << std::endl;
					#endif
					found = true;
				} else {
					#if TRACE_VNS2 >= 3
					std::cout << "VNS - Shake: rejected exchange" << std::endl;
					#endif
				}
			} else {
				CHECK(false);
			}
		}
		if (!found) {
			#if TRACE_VNS2 >= 2
			std::cout << "VNS - Shake failed after ";
			std::cout << i << "/" << k << " moves, objective = ";
			std::cout << m_current->objective() << std::endl;
			#endif
			stopped = true;
			break;
		}
	}
	if (!stopped) {
		#if TRACE_VNS2 >= 2
		std::cout << "VNS - Shake completed " << k << " moves, objective = ";
		std::cout << m_current->objective() << std::endl;
		#endif
	}
}

uint64_t VNS2::defaultMaxTrialsLS() {
	const double pCount = static_cast<double>(instance().processes().size());
	const double mCount = static_cast<double>(instance().processes().size());
	return static_cast<uint64_t>(pCount * (std::log10(pCount) + std::log10(mCount)));
}

void VNS2::run() {
	// read parameters
	m_useAdvisor = param<bool>("useAdvisor", false);
	m_useDynamicAdvisor = param<bool>("useDynamicAdvisor", false);
	m_kMin = param<uint64_t>("kMin", 1);
	m_kMax = param<uint64_t>("kMax", 100);
	m_maxTrialsLS = param<uint64_t>("maxTrialsLS", defaultMaxTrialsLS());
	m_maxTrialsShake = param<uint64_t>("maxTrialsShake", 1000);
	uint64_t syncPeriod = 10;
	// initialize
	m_rng.seed(seed());
	m_pDist = ProcessDist(0, instance().processes().size() - 1);
	m_mDist = MachineDist(0, instance().machines().size() - 1);
	m_best.reset(new SolutionInfo(instance(), initial()));
	if (m_useAdvisor) {
		m_advisor.reset(new Advisor(instance(), initial(), m_rng));
	}
	m_moveFeasibleEvalCount = 0;
	m_moveObjectiveEvalCount = 0;
	m_moveCommitCount = 0;
	m_moveEvalLocalSearch = 0;
	m_moveEvalShake = 0;
	m_moveCommitLocalSearch = 0;
	m_moveCommitShake = 0;
	m_exchangeFeasibleEvalCount = 0;
	m_exchangeObjectiveEvalCount = 0;
	m_exchangeCommitCount = 0;
	m_exchangeEvalLocalSearch = 0;
	m_exchangeEvalShake = 0;
	m_exchangeCommitLocalSearch = 0;
	m_exchangeCommitShake = 0;
	uint64_t k = m_kMin;
	uint64_t it = 0;
	uint64_t improvements = 0;
	// VNS iterations
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
		shake(k);
		// perform local search
		localSearch();
		// check improvement
		if (m_current->objective() < m_best->objective()) {
			#if TRACE_VNS2 >= 1
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
				++k;
			}
		}
	}
	#if TRACE_VNS2 >= 1
	std::cout << "Iterations: " << it << std::endl;
	std::cout << "Iterations with improvement: " << improvements << std::endl;
	std::cout << "Feasibility evaluations: ";
	std::cout << m_moveFeasibleEvalCount << " moves and ";
	std::cout << m_exchangeFeasibleEvalCount << " exchanges" << std::endl;
	std::cout << "Objective evaluations: ";
	std::cout << m_moveObjectiveEvalCount << " moves and ";
	std::cout << m_exchangeObjectiveEvalCount << " exchanges" << std::endl;
	std::cout << "Commits: ";
	std::cout << m_moveCommitCount << " moves and ";
	std::cout << m_exchangeCommitCount << " exchanges" << std::endl;
	std::cout << "Local search commit ratio: ";
	std::cout << moveCommitRatioLocalSearch() << " (move), ";
	std::cout << exchangeCommitRatioLocalSearch() << " (exchange)" << std::endl;
	std::cout << "Shake commit ratio: ";
	std::cout << moveCommitRatioShake() << " (move), ";
	std::cout << exchangeCommitRatioShake() << " (exchange)" << std::endl;
	#endif
	signalCompletion();
}
