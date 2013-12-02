#include "common.h"
#include "problem.h"
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <limits>
#include <utility>

using namespace R12;
using namespace std;

void Problem::parseResources(Problem & instance, std::vector<uint32_t>::const_iterator & it) {
	ResourceCount rCount = static_cast<ResourceCount>(*it);
	++it;
	instance.m_resources.reserve(rCount);
	for (ResourceID i = 0; i < rCount; ++i) {
		Resource r;
		r.setTransient(*it > 0);
		++it;
		r.setWeightLoadCost(*it);
		++it;
		instance.m_resources.push_back(r);
		if (r.transient()) {
			instance.m_transientResources.push_back(i);
		} else {
			instance.m_nonTransientResources.push_back(i);
		}
	}
}

void Problem::parseMachines(Problem & instance, std::vector<uint32_t>::const_iterator & it) {
	ResourceID rCount = static_cast<ResourceID>(instance.resources().size());
	MachineID mCount = static_cast<MachineID>(*it);
	++it;
	instance.m_machines.reserve(mCount);
	instance.m_locationCount = 0;
	instance.m_neighborhoodCount = 0;
	uint32_t maxMoveCost = std::numeric_limits<uint32_t>::min();
	instance.m_machineMoveCost.resize(mCount * mCount);
	for (MachineID i = 0; i < mCount; ++i) {
		Machine m(rCount);
		m.setNeighborhood(static_cast<NeighborhoodID>(*it));
		++it;
		m.setLocation(static_cast<LocationID>(*it));
		++it;
		for (ResourceID r = 0; r < rCount; ++r) {
			m.setCapacity(r, *it);
			++it;
		}
		for (ResourceID r = 0; r < rCount; ++r) {
			m.setSafetyCapacity(r, *it);
			++it;
		}
		for (MachineID j = 0; j < mCount; ++j) {
			uint32_t cost = *it;
			maxMoveCost = std::max(cost, maxMoveCost);
			instance.m_machineMoveCost[i * mCount + j] = cost;
			++it;
		}
		instance.m_machines.push_back(m);
		instance.m_locationCount = std::max(instance.m_locationCount, static_cast<LocationID>(m.location() + 1));
		instance.m_neighborhoodCount = std::max(instance.m_neighborhoodCount, static_cast<NeighborhoodID>(m.neighborhood() + 1));
	}
	if (maxMoveCost < 255u) {
		instance.m_useSmallMachineMoveCost = true;
		instance.m_smallMachineMoveCost.resize(mCount * mCount);
		std::copy(
			instance.m_machineMoveCost.begin(),
			instance.m_machineMoveCost.end(),
			instance.m_smallMachineMoveCost.begin());
		instance.m_machineMoveCost.clear();
	} else {
		instance.m_useSmallMachineMoveCost = false;
	}
	instance.m_machinesByLocation.resize(instance.m_locationCount);
	for (MachineID i = 0; i < mCount; ++i) {
		Machine & machine = instance.m_machines[i];
		instance.m_machinesByLocation[machine.location()].push_back(i);
	}
	instance.m_machinesByNeighborhood.resize(instance.m_neighborhoodCount);
	for (MachineID i = 0; i < mCount; ++i) {
		Machine & machine = instance.m_machines[i];
		instance.m_machinesByNeighborhood[machine.neighborhood()].push_back(i);
	}
}

#include <iostream>

void Problem::parseServices(Problem & instance, std::vector<uint32_t>::const_iterator & it) {
	ServiceCount sCount = static_cast<ServiceID>(*it);
	++it;
	instance.m_services.reserve(sCount);
	instance.m_processesByService.resize(sCount);
	std::vector<std::pair<int,int>> deps;
	for (ServiceID i = 0; i < sCount; ++i) {
		Service service;
		instance.m_services.push_back(service);
	}
	for (ServiceID i = 0; i < sCount; ++i) {
		Service & service = instance.m_services[i];
		service.setSpreadMin(*it);
		++it;
		// add service i outgoing edges to the edge list
		uint32_t dependencyCount = *it;
		++it;
		for (uint32_t d = 0; d < dependencyCount; ++d) {
			ServiceID j = static_cast<ServiceID>(*it);
			++it;
			deps.push_back(std::pair<ServiceID,ServiceID>(i,j));
		}
	}
	// create dependency graph and clear edge list
	instance.m_dep = DependencyGraph(
		boost::edges_are_unsorted_multi_pass,
		deps.begin(), deps.end(),
		sCount);
	// mark services with no in or out dependencies
	instance.m_serviceNoInDep.resize(sCount);
	instance.m_serviceNoOutDep.resize(sCount);
	for (ServiceID s = 0; s < sCount; ++s) {
		ServiceCount in = boost::in_degree(s, instance.dependency());
		ServiceCount out = boost::out_degree(s, instance.dependency());
		if (in == 0) {
			instance.m_serviceNoInDep[s] = true;
		}
		if (out == 0) {
			instance.m_serviceNoOutDep[s] = true;
		}
	}
}

void Problem::parseProcesses(Problem & instance, std::vector<uint32_t>::const_iterator & it) {
	const ResourceCount rCount = instance.resources().size();
	const ServiceCount sCount = instance.services().size();
	const ProcessCount pCount = *it;
	++it;
	instance.m_processes.reserve(pCount);
	for (ProcessID i = 0; i < pCount; ++i) {
		Process process(rCount);
		process.setService(static_cast<ServiceID>(*it));
		++it;
		for (ResourceID r = 0; r < rCount; ++r) {
			process.setRequirement(r, *it);
			++it;
		}
		process.setMovementCost(*it);
		++it;
		instance.m_processes.push_back(process);
		instance.m_processesByService[process.service()].push_back(i);
	}
	instance.m_serviceSingleProc.resize(sCount);
	for (ServiceID s = 0; s < sCount; ++s) {
		if (instance.m_processesByService[s].size() == 1) {
			instance.m_serviceSingleProc[s] = true;
		}
	}
}

void Problem::parseBalanceCosts(Problem & instance, std::vector<uint32_t>::const_iterator & it) {
	BalanceCostID bCount = static_cast<BalanceCostID>(*it);
	++it;
	instance.m_balanceCosts.reserve(bCount);
	for (BalanceCostID i = 0; i < bCount; ++i) {
		BalanceCost balanceCost;
		balanceCost.setResource1(static_cast<ResourceID>(*it));
		++it;
		balanceCost.setResource2(static_cast<ResourceID>(*it));
		++it;
		balanceCost.setTarget(*it);
		++it;
		balanceCost.setWeight(*it);
		++it;
		instance.m_balanceCosts.push_back(balanceCost);
	}
}

Problem Problem::parse(const vector<uint32_t> & values) {
	Problem instance;
	std::vector<uint32_t>::const_iterator vit = values.begin();
	parseResources(instance, vit);
	parseMachines(instance, vit);
	parseServices(instance, vit);
	parseProcesses(instance, vit);
	parseBalanceCosts(instance, vit);
	instance.m_weightProcessMoveCost = *vit;
	++vit;
	instance.m_weightServiceMoveCost = *vit;
	++vit;
	instance.m_weightMachineMoveCost = *vit;
	++vit;
	CHECK(vit == values.end());
	// compute lower bounds
	instance.m_lbLoadCost.resize(instance.m_resources.size());
	for (ResourceID r = 0; r < instance.m_resources.size(); ++r) {
		int64_t totalSafetyCap = 0;
		for (MachineID m = 0; m < instance.m_machines.size(); ++m) {
			totalSafetyCap += instance.m_machines[m].safetyCapacity(r);
		}
		int64_t totalReq = 0;
		for (ProcessID p = 0; p < instance.m_processes.size(); ++p) {
			totalReq += instance.m_processes[p].requirement(r);	
		}
		instance.m_lbLoadCost[r] = std::max<int64_t>(0, totalReq - totalSafetyCap);
	}
	instance.m_lbBalanceCost.resize(instance.m_balanceCosts.size());
	for (BalanceCostID b = 0; b < instance.m_balanceCosts.size(); ++b) {
		const BalanceCost & balance = instance.m_balanceCosts[b];
		const ResourceID r1 = balance.resource1();
		const ResourceID r2 = balance.resource2();
		int64_t totalCap1 = 0;
		int64_t totalCap2 = 0;
		for (MachineID m = 0; m < instance.m_machines.size(); ++m) {
			const Machine & machine = instance.m_machines[m];
			totalCap1 += machine.capacity(r1);
			totalCap2 += machine.capacity(r2);
		}
		int64_t totalReq1 = 0;
		int64_t totalReq2 = 0;
		for (ProcessID p = 0; p < instance.m_processes.size(); ++p) {
			const Process & process = instance.m_processes[p];
			totalReq1 += process.requirement(r1);
			totalReq2 += process.requirement(r2);
		}
		int64_t delta1 = totalCap1 - totalReq1;
		int64_t delta2 = totalCap2 - totalReq2;
		instance.m_lbBalanceCost[b] = std::max<int64_t>(0, balance.target() * delta1 - delta2);
	}
	return instance;
}

