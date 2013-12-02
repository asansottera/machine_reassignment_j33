#ifndef R12_BATCH_VERIFIER_H
#define R12_BATCH_VERIFIER_H

#include "move.h"
#include "solution_info.h"
#include "conflict_violation.h"
#include "dependency_violation.h"
#include <vector>
#include <unordered_set>
#include <set>
#include <boost/version.hpp>
#if BOOST_VERSION < 104800
#include <boost/interprocess/containers/flat_set.hpp>
#else
#include <boost/container/flat_set.hpp>
#endif

namespace R12 {

/*! Verifies constraints violation and computes solution objective after multiple moves. Moves can visit both feasible and infeasible solutions. */
class BatchVerifier {
public:
	typedef boost::container::flat_set<MachineID> CapacityViolationSet;
	typedef boost::container::flat_set<ConflictViolation> ConflictViolationSet;
	typedef boost::container::flat_set<ServiceID> SpreadViolationSet;
	typedef boost::container::flat_set<DependencyViolation> DependencyViolationSet;
private:
	SolutionInfo & m_info;
	mutable bool m_feasible;
	mutable uint64_t m_objective;
	mutable bool m_feasibleCached;
	mutable uint64_t m_objectiveCached;
	mutable std::vector<Move> m_movesToCheck;
	mutable CapacityViolationSet m_capacityViolations;
	mutable CapacityViolationSet m_transientViolations;
	mutable ConflictViolationSet m_conflictViolations;
	mutable SpreadViolationSet m_spreadViolations;
	mutable DependencyViolationSet m_dependencyViolations;
private:
	bool checkCapacity(MachineID m) const;
	bool checkTransient(MachineID m) const;
	bool checkFeasible() const;
	bool checkFeasibleIfNotCached() const {
		if (!m_feasibleCached) {
			m_feasible = checkFeasible();
			m_feasibleCached = true;
		}
		return m_feasible;
	}
	uint64_t computeObjective() const;
	SolutionInfo & info() { return m_info; }
public:
	BatchVerifier(SolutionInfo & info);
public:
	/*! Returns a read-only reference to set of capacity violations. Feasibility tracking must be enabled. */
	const CapacityViolationSet & capacityViolations() const {
		checkFeasibleIfNotCached();
		return m_capacityViolations;
	}
	/*! Returns a read-only reference to set of transient capacity violations. Feasibility tracking must be enabled. */
	const CapacityViolationSet & transientViolations() const {
		checkFeasibleIfNotCached();
		return m_transientViolations;
	}
	/*! Returns a read-only reference to set of conflict violations. Feasibility tracking must be enabled. */
	const ConflictViolationSet & conflictViolations() const {
		checkFeasibleIfNotCached();
		return m_conflictViolations;
	}
	/*! Returns a read-only reference to set of spread violations. Feasibility tracking must be enabled. */
	const SpreadViolationSet & spreadViolations() const {
		checkFeasibleIfNotCached();
		return m_spreadViolations;
	}
	/*! Returns a read-only reference to set of dependency violations. Feasibility tracking must be enabled. */
	const DependencyViolationSet & dependencyViolations() const {
		checkFeasibleIfNotCached();
		return m_dependencyViolations;
	}
public:
	/*! Returns a read-only reference to the information about the current solution. */
	const SolutionInfo & info() const { return m_info; }
	/*! Returns a read-only reference to the problem instance data. */
	const Problem & instance() const { return m_info.instance(); }
	/*! Returns a read-only reference to the assignment vector of the iniitial solution. */
	const std::vector<MachineID> & initial() const { return m_info.initial(); }
	/*! Returns a read-only reference to the assignment vector. */
	const std::vector<MachineID> & solution() const { return m_info.solution(); }
	/*! Applies multiple moves. */
	void update(const std::vector<Move> & moves);
	/*! Applies a single move. */
	void update(const Move & move);
	/*! Cancels multiple moves. */
	void rollback(const std::vector<Move> & moves);
	/*! Cancels a single move. */
	void rollback(const Move & move);
	/*! Checks whether the solution is feasible or not. */
	bool feasible() const {
		return checkFeasibleIfNotCached();
	}
	/*! Computes the objective function. */
	uint64_t objective() const {
		if (!m_objectiveCached) {
			m_objective = computeObjective();
			m_objectiveCached = true;
		}
		return m_objective;
	}

};

}

#endif
