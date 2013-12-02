#include "common.h"
#include "verifier.h"
#include <unordered_set>
#include <unordered_map>

using namespace R12;
using namespace std;

bool Verifier::verifyCapacity(const Problem & instance, const std::vector<MachineID> & solution) {
	bool feasible = true;
	urm.resize(instance.resources().size() * instance.machines().size());
	for (ProcessID p = 0; p < instance.processes().size(); ++p) {
		for (ResourceID r = 0; r < instance.resources().size(); ++r) {
			// add process contribution to resource utilization on the machine it is placed on
			MachineID m = solution[p];
			int req = instance.processes()[p].requirement(r);
			int cap = instance.machines()[m].capacity(r);
			urm[m * instance.resources().size() + r] += req;
			if (urm[m * instance.resources().size() + r] > cap) {
				feasible = false;
				return feasible;
			}
		}
	}
	return feasible;
}

bool Verifier::verifyConflict(const Problem & instance, const std::vector<MachineID> & solution) {
	bool feasible = true;
	for (ProcessID p1 = 0; p1 < instance.processes().size(); ++p1) {
		for (ProcessID p2 = 0; p2 < instance.processes().size(); ++p2) {
			ServiceID s1 = instance.processes()[p1].service();
			ServiceID s2 = instance.processes()[p2].service();
			MachineID m1 = solution[p1];
			MachineID m2 = solution[p2];
			if (p1 != p2 && s1 == s2 && m1 == m2) {
				feasible = false;
				return feasible;
			}
		}
	}
	return feasible;
}

bool Verifier::verifySpread(const Problem & instance, const std::vector<MachineID> & solution) {
	bool feasible = true;
	typedef unordered_map<ServiceID,unordered_set<LocationID>> S2L;
	S2L serviceToLocations;
	for (ProcessID p = 0; p < instance.processes().size(); ++p) {
		ServiceID s = instance.processes()[p].service();
		MachineID m = solution[p];
		LocationID l = instance.machines()[m].location();
		S2L::iterator it = serviceToLocations.find(s);
		if (it == serviceToLocations.end()) {
			unordered_set<LocationID> locations;
			locations.insert(l);
			serviceToLocations.insert(S2L::value_type(s, locations));
		} else {
			it->second.insert(l);
		}
	}
	for (ServiceID s = 0; s < instance.services().size(); ++s) {
		uint32_t spread = 0;
		S2L::iterator it = serviceToLocations.find(s);
		if (it != serviceToLocations.end()) {
			spread = it->second.size();
		}
		if (spread < instance.services()[s].spreadMin()) {
			feasible = false;
			return feasible;
		}
	}
	return feasible;
}

bool Verifier::verifyDependency(const Problem & instance, const std::vector<MachineID> & solution) {
	bool feasible = true;
	for (ServiceID s = 0; s < instance.services().size(); ++s) {
		auto deps = boost::out_edges(s, instance.dependency());
		const std::vector<ProcessID> & processes = instance.processesByService(s);
		if (deps.first != deps.second) {
			// each process of the current service must have dependencies close to it
			for (auto pItr = processes.begin(); pItr != processes.end(); ++pItr) {
				ProcessID p = *pItr;
				MachineID pMachine = solution[p];
				NeighborhoodID pNeighborhood = instance.machines()[pMachine].neighborhood();
				// for each server on which the current service depends on
				for (auto depItr = deps.first; depItr != deps.second; ++depItr) {
					ServiceID dep = boost::target(*depItr, instance.dependency());
					const std::vector<ProcessID> & depProcesses = instance.processesByService(dep);
					bool foundInSameNeighborhood = false;
					// at least one process in the same neighborhood
					for (auto depProcessItr = depProcesses.begin(); depProcessItr != depProcesses.end(); ++depProcessItr) {
						ProcessID depProcess = *depProcessItr;
						MachineID depMachine = solution[depProcess];
						NeighborhoodID depNeighborhood = instance.machines()[depMachine].neighborhood();
						if (pNeighborhood == depNeighborhood) {
							foundInSameNeighborhood = true;
							break;
						}
					}
					if (!foundInSameNeighborhood) {
						feasible = false;
						return feasible;
					}
				}
			}
		}
	}
	return feasible;
}

bool Verifier::verifyTransientUsage(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution) {
	bool feasible = true;
	for (ResourceID r = 0; r < instance.resources().size(); ++r) {
		if (instance.resources()[r].transient()) {
			// compute transient usage of all machines
			std::vector<uint32_t> req(instance.machines().size(), 0);
			for (ProcessID p = 0; p < instance.processes().size(); ++p) {
				const Process & process = instance.processes()[p];
				MachineID mInitial = initial[p];
				MachineID mSolution = solution[p];
				req[mInitial] += process.requirement(r);
				if (mSolution != mInitial) {
					req[mSolution] += process.requirement(r);
				}
			}
			// check transient usage below capacity
			for (MachineID m = 0; m < instance.machines().size(); ++m) {
				const Machine & machine = instance.machines()[m];
				if (req[m] > machine.capacity(r)) {
					feasible = false;
					return feasible;
				}
			}
		}
	}
	return feasible;
}

uint64_t Verifier::evaluateLoadCosts(const Problem & instance, const std::vector<MachineID> & solution) {
	uint64_t loadCost = 0;
	for (ResourceID r = 0; r < instance.resources().size(); ++r) {
		uint64_t resourceLoadCost = 0;
		for (MachineID m = 0; m < instance.machines().size(); ++m) {
			int64_t safetyCapacity = instance.machines()[m].safetyCapacity(r);
			int64_t load = urm[m * instance.resources().size() + r];
			resourceLoadCost += std::max((int64_t)0, load - safetyCapacity);
		}
		loadCost += resourceLoadCost * instance.resources()[r].weightLoadCost();
	}
	return loadCost;
}

uint64_t Verifier::evaluateBalanceCosts(const Problem & instance, const std::vector<MachineID> & solution) {
	uint64_t totalCost = 0;
	for (BalanceCostID bc = 0; bc < instance.balanceCosts().size(); ++bc) {
		const BalanceCost & balanceCost = instance.balanceCosts()[bc];
		ResourceID r1 = balanceCost.resource1();
		ResourceID r2 = balanceCost.resource2();
		uint64_t cost = 0;
		for (MachineID m = 0; m < instance.machines().size(); ++m) {
			const Machine & machine = instance.machines()[m];
			int64_t ar1 = static_cast<int64_t>(machine.capacity(r1)) - static_cast<int64_t>(urm[m * instance.resources().size() + r1]);
			int64_t ar2 = static_cast<int64_t>(machine.capacity(r2)) - static_cast<int64_t>(urm[m * instance.resources().size() + r2]);
			cost += std::max((int64_t)0, balanceCost.target() * ar1 - ar2);
		}
		totalCost += balanceCost.weight() * cost;
	}
	return totalCost;
}

uint64_t Verifier::evaluateProcessMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution) {
	uint64_t totalProcessMoveCost = 0;
	for (ProcessID p = 0; p < instance.processes().size(); ++p) {
		if (initial[p] != solution[p]) {
			const Process & process = instance.processes()[p];
			totalProcessMoveCost += process.movementCost();
		}
	}
	return instance.weightProcessMoveCost() * totalProcessMoveCost;
}

uint64_t Verifier::evaluateServiceMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution) {
	uint64_t serviceMoveCost = 0;
	for (ServiceID s = 0; s < instance.services().size(); ++s) {
		uint64_t movedCount = 0;
		const std::vector<ProcessID> & sProcesses = instance.processesByService(s);
		for (auto itr = sProcesses.begin(); itr != sProcesses.end(); ++itr) {
			ProcessID p = *itr;
			if (initial[p] != solution[p]) {
				++movedCount;
			}
		}
		serviceMoveCost = std::max(serviceMoveCost, movedCount);
	}
	return instance.weightServiceMoveCost() * serviceMoveCost;
}

uint64_t Verifier::evaluateMachineMoveCost(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution) {
	uint64_t machineMoveCost = 0;
	for (ProcessID p = 0; p < instance.processes().size(); ++p) {
		MachineID sourceMachineID = initial[p];
		MachineID destMachineID = solution[p];
		machineMoveCost += instance.machineMoveCost(sourceMachineID, destMachineID);
	}
	return instance.weightMachineMoveCost() * machineMoveCost;
}

Verifier::Result Verifier::verify(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution) {
	bool feasible = true;
	urm.clear();
	feasible = verifyCapacity(instance, solution);
	if (!feasible) return Result(feasible, 0);
	feasible = verifyConflict(instance, solution);
	if (!feasible) return Result(feasible, 0);
	feasible = verifySpread(instance, solution);
	if (!feasible) return Result(feasible, 0);
	feasible = verifyDependency(instance, solution);
	if (!feasible) return Result(feasible, 0);
	feasible = verifyTransientUsage(instance, initial, solution);
	if (!feasible) return Result(feasible, 0);
	uint64_t objective = 0;
	objective += evaluateLoadCosts(instance, solution);
	objective += evaluateBalanceCosts(instance, solution);
	objective += evaluateProcessMoveCost(instance, initial, solution);
	objective += evaluateServiceMoveCost(instance, initial, solution);
	objective += evaluateMachineMoveCost(instance, initial, solution);
	return Result(feasible, objective);
}

