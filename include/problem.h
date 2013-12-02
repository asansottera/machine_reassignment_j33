#ifndef R12_PROBLEM_H
#define R12_PROBLEM_H

#include "common.h"
#include <vector>
#include <boost/graph/compressed_sparse_row_graph.hpp>

namespace R12 {

/*! Represents a computation resource. */
class Resource {
private:
	bool m_transient;
	uint32_t m_weightLoadCost;
public:
	bool transient() const { return m_transient; }
	void setTransient(bool transient) { m_transient = transient; }
	uint32_t weightLoadCost() const { return m_weightLoadCost; }
	void setWeightLoadCost(uint32_t weightLoadCost) { m_weightLoadCost = weightLoadCost; }
};

/*! Represents a machine. */
class Machine {
private:
	NeighborhoodID m_neighborhood;
	LocationID m_location;
	std::vector<uint32_t> m_capacity;
	std::vector<uint32_t> m_safetyCapacity;
public:
	Machine(ResourceID resourceCount) {
		m_capacity.resize(resourceCount);
		m_safetyCapacity.resize(resourceCount);
	}
	NeighborhoodID neighborhood() const { return m_neighborhood; }
	void setNeighborhood(NeighborhoodID neighborhood) { m_neighborhood = neighborhood; }
	LocationID location() const { return m_location; }
	void setLocation(LocationID location) { m_location = location; }
	uint32_t capacity(ResourceID resource) const { return m_capacity[resource]; }
	void setCapacity(ResourceID resource, uint32_t capacity) { m_capacity[resource] = capacity; }
	uint32_t safetyCapacity(ResourceID resource) const { return m_safetyCapacity[resource]; }
	void setSafetyCapacity(ResourceID resource, uint32_t safetyCapacity) { m_safetyCapacity[resource] = safetyCapacity; }
};

/*! Represents a service. */
class Service {
private:
	uint32_t m_spreadMin;
public:
	Service() {
	}
	uint32_t spreadMin() const { return m_spreadMin; }
	void setSpreadMin(uint32_t spreadMin) { m_spreadMin = spreadMin; }
};

/*! Represents a process. */
class Process {
private:
	ServiceID m_service;
	std::vector<uint32_t> m_requirement;
	uint32_t m_movementCost;
public:
	Process(ResourceID rCount) {
		m_requirement.resize(rCount);
	}
	ServiceID service() const { return m_service; }
	void setService(ServiceID service) { m_service = service; }
	uint32_t requirement(ResourceID resource) const { return m_requirement[resource]; }
	void setRequirement(ResourceID resource, uint32_t requirement) { m_requirement[resource] = requirement; }
	uint32_t movementCost() const { return m_movementCost; }
	void setMovementCost(uint32_t movementCost) { m_movementCost = movementCost; }
};

/*! Represents a balance objective. */
class BalanceCost {
private:
	ResourceID m_resource1;
	ResourceID m_resource2;
	uint32_t m_target;
	uint32_t m_weight;
public:
	ResourceID resource1() const { return m_resource1; }
	void setResource1(ResourceID resource1) { m_resource1 = resource1; }
	ResourceID resource2() const { return m_resource2; }
	void setResource2(ResourceID resource2) { m_resource2 = resource2; }
	uint32_t target() const { return m_target; }
	void setTarget(uint32_t target) { m_target = target; }
	uint32_t weight() const { return m_weight; }
	void setWeight(uint32_t weight) { m_weight = weight; }
};

/*! Represents an instance of the ROADEF 2012 problem. */
class Problem {
public:
	typedef boost::compressed_sparse_row_graph<
		boost::bidirectionalS,
		boost::no_property,
		boost::no_property,
		boost::no_property,
		ServiceID> DependencyGraph;
private:
	LocationID m_locationCount;
	NeighborhoodID m_neighborhoodCount;
	std::vector<Resource> m_resources;
	std::vector<Machine> m_machines;
	std::vector<Service> m_services;
	std::vector<Process> m_processes;
	std::vector<BalanceCost> m_balanceCosts;
	std::vector<bool> m_serviceSingleProc;
	std::vector<bool> m_serviceNoInDep;
	std::vector<bool> m_serviceNoOutDep;
	uint32_t m_weightProcessMoveCost;
	uint32_t m_weightServiceMoveCost;
	uint32_t m_weightMachineMoveCost;
	std::vector<std::vector<ProcessID>> m_processesByService;
	std::vector<std::vector<MachineID>> m_machinesByLocation;
	std::vector<std::vector<MachineID>> m_machinesByNeighborhood;
	std::vector<ResourceID> m_nonTransientResources;
	std::vector<ResourceID> m_transientResources;
	DependencyGraph m_dep;
	std::vector<uint64_t> m_lbLoadCost;
	std::vector<uint64_t> m_lbBalanceCost;
	bool m_useSmallMachineMoveCost;
	std::vector<uint32_t> m_machineMoveCost;
	std::vector<uint8_t> m_smallMachineMoveCost;
private:
	Problem() {
	}
public:
	uint32_t machineMoveCost(const MachineID m1, const MachineID m2) const {
		const std::size_t index = m1 * m_machines.size() + m2;
		if (m_useSmallMachineMoveCost) {
			CHECK(m_machineMoveCost.size() == 0);
			return m_smallMachineMoveCost[index];
		} else {
			CHECK(m_smallMachineMoveCost.size() == 0);
			return m_machineMoveCost[index];
		}
	}
	uint64_t lowerBoundLoadCost(const ResourceID r) const {
		return m_lbLoadCost[r];
	}
	uint64_t lowerBoundBalanceCost(const BalanceCostID b) const {
		return m_lbBalanceCost[b];
	}
	uint64_t lowerBoundObjective() const {
		uint64_t lb = 0;
		for (ResourceID r = 0; r < m_resources.size(); ++r) {
			const Resource & resource = m_resources[r];
			lb += lowerBoundLoadCost(r) * resource.weightLoadCost();
		}
		for (BalanceCostID b = 0; b < m_balanceCosts.size(); ++b) {
			const BalanceCost & balance = m_balanceCosts[b];
			lb += lowerBoundBalanceCost(b) * balance.weight();
		}
		return lb;
	}
	/*! Returns true if the service has a single process. */
	bool serviceHasSingleProcess(const ServiceID s) const { return m_serviceSingleProc[s]; }
	/*! Returns true if the service has no other service depending on it. */
	bool serviceHasNoInDependency(const ServiceID s) const { return m_serviceNoInDep[s]; }
	/*! Returns true if the service depends on no other service. */
	bool serviceHasNoOutDependency(const ServiceID s) const { return m_serviceNoOutDep[s]; }
	/*! Returns the number of locations. */
	LocationCount locationCount() const { return m_locationCount; }
	/*! Returns the number of neighborhoods. */
	NeighborhoodCount neighborhoodCount() const { return m_neighborhoodCount; }
	/*! Returns a constant reference to the vector of resources. */
	const std::vector<Resource> & resources() const { return m_resources; }
	/*! Returns a constant reference to the vector of machines. */
	const std::vector<Machine> & machines() const { return m_machines; }
	/*! Returns a constant reference to the vector of services. */
	const std::vector<Service> & services() const { return m_services; }
	/*! Returns a constant reference to the vector of processes. */
	const std::vector<Process> & processes() const { return m_processes; }
	/*! Returns a constant reference to the vector of balance objectives. */
	const std::vector<BalanceCost> & balanceCosts() const { return m_balanceCosts; }
	/*! Returns the weight of process move cost. */
	uint32_t weightProcessMoveCost() const { return m_weightProcessMoveCost; }
	/*! Returns the weight of service move cost. */
	uint32_t weightServiceMoveCost() const { return m_weightServiceMoveCost; }
	/*! Returns the weight of machine move cost. */
	uint32_t weightMachineMoveCost() const { return m_weightMachineMoveCost; }
	/*! Returns the processes which belong to a service. */
	const std::vector<ProcessID> & processesByService(ServiceID service) const { return m_processesByService[service]; }
	/*! Returns the machines in a location: */
	const std::vector<MachineID> & machinesByLocation(LocationID location) const { return m_machinesByLocation[location]; }
	/*! Returns the machines in a neighborhood. */
	const std::vector<MachineID> & machinesByNeighborhood(NeighborhoodID neighborhood) const { return m_machinesByNeighborhood[neighborhood]; }
	/*! Returns the resources which are non transient. */
	const std::vector<ResourceID> & nonTransientResources()  const { return m_nonTransientResources; }
	/*! Returns the resources which are transient. */
	const std::vector<ResourceID> & transientResources() const { return m_transientResources; }
	/*! Returns the graph representing the dependency relation among services. */
	const DependencyGraph & dependency() const { return m_dep; };
private:
	static void parseResources(Problem & instance, std::vector<uint32_t>::const_iterator & it);
	static void parseMachines(Problem & instance, std::vector<uint32_t>::const_iterator & it);
	static void parseServices(Problem & instance, std::vector<uint32_t>::const_iterator & it);
	static void parseProcesses(Problem & instance, std::vector<uint32_t>::const_iterator & it);
	static void parseBalanceCosts(Problem & instance, std::vector<uint32_t>::const_iterator & it);
public:
	/*! Reads the problem instance from a vector of integer values, representing its parameters. */
	static Problem parse(const std::vector<uint32_t> & values);
};

}

#endif
