#ifndef R12_SOLUTION_INFO_H
#define R12_SOLUTION_INFO_H

#include "problem.h"
#include <vector>
#include <algorithm>
#include <iostream>

namespace R12 {

/*! Containts information about a solution. */
class SolutionInfo {
private:
	const Problem * m_instancePtr;
	const std::vector<MachineID> * m_initialPtr;
private:
	std::vector<MachineID> m_solution;
	std::vector<std::vector<uint32_t>> m_usage;
	std::vector<std::vector<uint32_t>> m_transient;
	std::vector<LocationCount> m_spread;
	std::vector<std::vector<bool>> m_boolMachinePresence;
	std::vector<std::vector<ProcessCount>> m_machinePresence;
	std::vector<std::vector<ProcessCount>> m_locationPresence;
	std::vector<std::vector<ProcessCount>> m_neighborhoodPresence;
	std::vector<ProcessCount> m_movedProcesses;
	std::vector<uint64_t> m_loadCosts;
	std::vector<uint64_t> m_balanceCosts;
	uint64_t m_processMoveCost;
	uint64_t m_serviceMoveCost;
	uint64_t m_machineMoveCost;
private:
	void initializeContainers();
	void initialize();
	void initializeSolutionDelta();
public:
	void computeServiceMoveCost() {
		m_serviceMoveCost = *std::max_element(m_movedProcesses.begin(), m_movedProcesses.end());
	}
public:
	const Problem & instance() const {
		return *m_instancePtr;
	}
	const std::vector<MachineID> & initial() const {
		return *m_initialPtr;
	}
	const std::vector<MachineID> & solution() const {
		return m_solution;
	}
	std::vector<MachineID> & solution() {
		return m_solution;
	}
public:
	SolutionInfo(const Problem & instance, const std::vector<MachineID> & initial);
	SolutionInfo(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution);
public:
	bool check() const;
	bool operator==(const SolutionInfo & other) const;
public:
	/*! Sets the usage of resource r on machine m. */
	void setUsage(MachineID m, ResourceID r, uint32_t value) {
		m_usage[m][r] = value;
	}
	/*! Sets the usage of resource r on machine m due to processes initially on m being moved somwhere else. */
	void setTransient(MachineID m, ResourceID r, uint32_t value) {
		m_transient[m][r] = value;
	}
	/*! Sets the number of processes of service s on machine m. */
	void setMachinePresence(ServiceID s, MachineID m, ProcessCount value) {
		m_machinePresence[s][m] = value;
	}
	/*! Sets whether any process of service s is on machine m. */
	void setBoolMachinePresence(ServiceID s, MachineID m, bool value) {
		m_boolMachinePresence[s][m] = value;
	}
	/*! Sets the number of processes of service s in the location l. */
	void setLocationPresence(ServiceID s, LocationID l, ProcessCount value) {
		uint32_t old = m_locationPresence[s][l];
		m_locationPresence[s][l] = value;
		if (old == 0 && value != 0) {
			++m_spread[s];
		} else if (old != 0 && value == 0) {
			--m_spread[s];
		}
	}
	/*! Sets the number of processes of service s in the neighborhood n. */
	void setNeighborhoodPresence(ServiceID s, NeighborhoodID n, ProcessCount value) {
		m_neighborhoodPresence[s][n] = value;
	}
	/*! Sets the number of processes of service s moved from their original assignment. */
	void setMovedProcesses(ServiceID s, ProcessCount value) {
		m_movedProcesses[s] = value;
	}
	/*! Sets the load cost of resource r. */
	void setLoadCost(ResourceID r, uint64_t value) {
		m_loadCosts[r] = value;
	}
	/*! Sets the value of balance cost b. */
	void setBalanceCost(BalanceCostID b, uint64_t value) {
		m_balanceCosts[b] = value;
	}
	/*! Sets the total process move cost. */
	void setProcessMoveCost(uint64_t value) {
		m_processMoveCost = value;
	}
	/*! Sets the service move cost. */
	void setServiceMoveCost(uint64_t value) {
		m_serviceMoveCost = value;
	}
	/*! Sets the total machine move cost. */
	void setMachineMoveCost(uint64_t value) {
		m_machineMoveCost = value;
	}
public:
	uint32_t usage(MachineID m, ResourceID r) const {
		return m_usage[m][r];
	}
	/*! Returns the usage of resource r on machine m due to processes initially on m being moved somewhere else. */
	uint32_t transient(MachineID m, ResourceID r) const {
		return m_transient[m][r];
	}
	/*! Returns the spread of service s, computed as the number of locations where there is at least one process. */
	LocationCount spread(ServiceID s) const {
		return m_spread[s];
	}
	/*! Returns the number of processes of service s on machine m. */
	ProcessCount machinePresence(ServiceID s, MachineID m) const {
		return m_machinePresence[s][m];
	}
	/*! Returns true if any process of service s is on machine m. */
	bool boolMachinePresence(ServiceID s, MachineID m) const {
		return m_boolMachinePresence[s][m];
	}
	/*! Returns the number of processes of service s in the location l. */
	ProcessCount locationPresence(ServiceID s, LocationID l) const {
		return m_locationPresence[s][l];
	}
	/*! Returns the number of processes of service s in the neighborhood n. */
	ProcessCount neighborhoodPresence(ServiceID s, NeighborhoodID n) const {
		return m_neighborhoodPresence[s][n];
	}
	/*! Returns sthe number of processes of service s moved from their original assignment. */
	ProcessCount movedProcesses(ServiceID s) const {
		return m_movedProcesses[s];
	}
	/*! Returns the load cost of resource r. */
	uint64_t loadCost(ResourceID r) const {
		return m_loadCosts[r];
	}
	/*! Returns the value of balance cost b. */
	uint64_t balanceCost(BalanceCostID b) const {
		return m_balanceCosts[b];
	}
	/*! Returns the total process move cost. */
	uint64_t processMoveCost() const {
		return m_processMoveCost;
	}
	/*! Returns the total service move cost. */
	uint64_t serviceMoveCost() const {
		return m_serviceMoveCost;
	}
	/*! Returns the total machine move cost. */
	uint64_t machineMoveCost() const {
		return m_machineMoveCost;
	}
	/*! Returns the total load cost. */
	uint64_t totalLoadCost() const {
		const ResourceCount rCount = instance().resources().size();
		uint64_t total = 0;
		for (ResourceID r = 0; r < rCount; ++r) {
			const Resource & resource = instance().resources()[r];
			uint64_t weight = resource.weightLoadCost();
			total += loadCost(r) * weight;
		}
		return total;
	}
	/*! Returns the total balance cost. */
	uint64_t totalBalanceCost() const {
		const BalanceCostCount bCount = instance().balanceCosts().size();
		uint64_t total = 0;
		for (BalanceCostID b = 0; b < bCount; ++b) {
			const BalanceCost & balance = instance().balanceCosts()[b];
			uint64_t weight = balance.weight();
			total += balanceCost(b) * weight;
		}
		return total;
	}
	/*! Returns the total cost due to process movement */
	uint64_t totalMoveCost() const {
		uint64_t total = 0;
		total += instance().weightProcessMoveCost() * processMoveCost();
		total += instance().weightServiceMoveCost() * serviceMoveCost();
		total += instance().weightMachineMoveCost() * machineMoveCost();
		return total;
	}
	/*! Returns the objective value. */
	uint64_t objective() const {
		uint64_t obj = totalLoadCost() + totalBalanceCost() + totalMoveCost();
		return obj;
	}
	float fractionLoadCost() const {
		float tlc = totalLoadCost();
		float tbc = totalBalanceCost();
		float tmc = totalMoveCost();
		return tlc / (tlc + tbc + tmc);
	}
	float fractionBalanceCost() const {
		float tlc = totalLoadCost();
		float tbc = totalBalanceCost();
		float tmc = totalMoveCost();
		return tbc / (tlc + tbc + tmc);
	}
	float fractionMoveCost() const {
		float tlc = totalLoadCost();
		float tbc = totalBalanceCost();
		float tmc = totalMoveCost();
		return tmc / (tlc + tbc + tmc);
	}
};

inline uint64_t _computeLoadCost(uint32_t usage, uint32_t safetyCapacity) {
	int64_t delta = static_cast<int64_t>(usage) - static_cast<int64_t>(safetyCapacity);
	return static_cast<uint64_t>(std::max((int64_t)0, delta));
}

inline uint64_t _computeBalanceCost(const BalanceCost & balance, const Machine & machine, uint32_t usage1, uint32_t usage2) {
	int64_t a1 = static_cast<int64_t>(machine.capacity(balance.resource1())) - static_cast<int64_t>(usage1);
	int64_t a2 = static_cast<int64_t>(machine.capacity(balance.resource2())) - static_cast<int64_t>(usage2);
	return static_cast<uint64_t>(std::max((int64_t)0, balance.target() * a1 - a2));
}

inline void writeCostComposition(SolutionInfo & x) {
	std::cout << "Objective = " << x.objective() << ", composition: ";
	std::cout << x.fractionLoadCost() * 100;
	std::cout << "\% load costs + ";
	std::cout << x.fractionBalanceCost() * 100;
	std::cout << "\% balance costs + ";
	std::cout << x.fractionMoveCost() * 100;
	std::cout << "\% move costs + ";
	std::cout << std::endl;
}

}

#endif
