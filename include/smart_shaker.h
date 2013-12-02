#ifndef R12_SMART_SHAKER_H
#define R12_SMART_SHAKER_H

#include "common.h"
#include "move.h"
#include "solution_info.h"
#include "atomic_flag.h"
#include "batch_verifier.h"
#include "greedy_advisor.h"
#include <memory>
#include <vector>
#include <set>
#include <boost/random.hpp>

namespace R12 {

/*! Implements jumps to a neighborhood Nk */
class SmartShaker {
private: // objects shared with other objects: do not destroy
	const Problem * m_instancePtr;
	const std::vector<MachineID> * m_initialPtr;
	const AtomicFlag * m_terminationFlagPtr;
private: // objects used during a shake operation
	SolutionInfo * m_infoPtr;
	std::unique_ptr<BatchVerifier> m_bvPtr;
	std::unique_ptr<GreedyAdvisor> m_advisorPtr;
	std::vector<std::set<ProcessID>> m_processesByMachine;
private: // distributions for random move selection
	boost::taus88 m_gen;
	boost::uniform_int<ProcessID> m_pdist;
	std::vector<boost::uniform_int<ProcessID>> m_pdistByService;
	boost::uniform_int<MachineID> m_mdist;
	std::vector<boost::uniform_int<MachineID>> m_mdistByLocation;
	std::vector<boost::uniform_int<MachineID>> m_mdistByNeighborhood;
	boost::uniform_int<LocationID> m_ldist;
	boost::uniform_real<float> m_realdist;
private: // statistics
	uint64_t m_shakes;
	uint64_t m_failureCount;
private: // options
	uint32_t m_globalTrials;
	uint32_t m_globalMaxInfeasibleSteps;
private: // just for convenience
	const Problem & instance() const {
		return *(m_instancePtr);
	}
	const std::vector<MachineID> & initial() const {
		return *(m_initialPtr);
	}
	const AtomicFlag & terminationFlag() const {
		return *(m_terminationFlagPtr);
	}
	BatchVerifier & bv() const {
		return *(m_bvPtr);
	}
	SolutionInfo & info() const {
		return *(m_infoPtr);
	}
	GreedyAdvisor & advisor() const {
		return *(m_advisorPtr);
	}
private:
	/*! Updates the list processes on each machine. */
	void updateProcessesByMachine(const Move & move);
	/*! Selects a random move */
	Move pickRandomMove();
	/*! Selects a move which tries to go toward the feasible region */
	Move pickRepairMove();
	/*! Selects a move which reduces the usage of a machine */
	Move pickRepairCapacityMove(const MachineID src);
	/*! Selects a move which increases the spread of a service */
	Move pickRepairSpreadMove(const ServiceID s);
	/*! Selects a move which tries to repair a conflict */
	Move pickRepairConflictMove(const ConflictViolation & cv);
	/*! Selects a move which tries to repair a dependency */
	Move pickRepairDependencyMove(const DependencyViolation & dv);
	/*! Jumps to neighborhood Nk passing through both feasible and unfeasible solutions */
	bool tryGlobalShake(const uint32_t k);
public:
	SmartShaker(uint32_t globalTrials, uint32_t maxInfeasibleSteps);
	/*! Initalizes the shaker. Must be called before calling shake, otherwise behavior is undefined. */
	void init(const Problem & instance, const std::vector<MachineID> & initial, uint32_t seed, const AtomicFlag & terminationFlag);
	/*! Jumps to neighborhood Nk */
	void shake(const uint32_t k, SolutionInfo & info);
	/*! Average number of failures per shake */
	double failuresPerShake() const {
		return static_cast<double>(m_failureCount) / static_cast<double>(m_shakes);
	}
};

}

#endif
