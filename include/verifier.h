#ifndef R12_VERIFIER_H
#define R12_VERIFIER_H

#include "problem.h"
#include <vector>
#include <cstdint>
#include <utility>

namespace R12 {

/*! An instance of this class verifies the feasibility of a problem solution. */
class Verifier {
private:
	std::vector<int> urm;
private:
	bool verifyCapacity(const Problem & instance, const std::vector<MachineID> & solution);
	bool verifyConflict(const Problem & instance, const std::vector<MachineID> & solution);
	bool verifySpread(const Problem & instance, const std::vector<MachineID> & solution);
	bool verifyDependency(const Problem & instance, const std::vector<MachineID> & solution);
	bool verifyTransientUsage(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
	uint64_t evaluateLoadCosts(const Problem & instance, const std::vector<MachineID> & solution);
	uint64_t evaluateBalanceCosts(const Problem & instance, const std::vector<MachineID> & soluytion);
	uint64_t evaluateProcessMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
	uint64_t evaluateServiceMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
	uint64_t evaluateMachineMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
public:
	class Result {
	private:
		bool m_feasible;
		uint64_t m_objective;
	public:
		Result(bool feasible, uint64_t objective) {
			m_feasible = feasible;
			m_objective = objective;
		}
		bool feasible() const { return m_feasible; }
		uint64_t objective() const { return m_objective; }
	};
	Result verify(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
};

}

#endif
