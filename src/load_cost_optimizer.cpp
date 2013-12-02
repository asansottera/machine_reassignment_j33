#include "load_cost_optimizer.h"
#include "move_verifier.h"
#include "vector_comparer.h"
#include "pair_comparer.h"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace R12;

#define TRACE_LCOPT 1

uint64_t LoadCostOptimizer::evaluateLoadCost(const Machine & machine,
											 const ResourceID r,
											 const int64_t usage) const {
	int64_t safetyCapacity = machine.safetyCapacity(r);
	uint64_t loadCost = std::max<int64_t>(0, usage - safetyCapacity);
	return loadCost;
}

void LoadCostOptimizer::optimizeResource(SolutionInfo & x,
										 const ResourceID r) const {
	typedef std::pair<MachineID,uint64_t> MLC;
	const Problem & instance = x.instance();
	const ProcessCount pCount = instance.processes().size();
	const MachineCount mCount = instance.machines().size();
	std::vector<MLC> sortedMachines;
	std::vector<MachineID> lowLoadMachines;
	for (MachineID m = 0; m < mCount; ++m) {
		const Machine & machine = instance.machines()[m];
		int64_t usage = x.usage(m, r);
		uint64_t loadCost = evaluateLoadCost(machine, r, usage);
		if (loadCost > 0) {
			sortedMachines.push_back(MLC(m, loadCost));
		} else {
			lowLoadMachines.push_back(m);
		}
	}
	#if TRACE_LCOPT >= 1
	std::cout << "Machines with positive load cost for resource " << r;
	std::cout << ": " << sortedMachines.size() << "/" << mCount << std::endl;
	#endif
	std::sort(sortedMachines.begin(),
			  sortedMachines.end(),
			  PairComparer<MachineID,uint64_t,true>());
	MoveVerifier mv(x);
	for (MachineCount i = 0; i < sortedMachines.size(); ++i) {
		const MachineID m = sortedMachines[i].first;
		const uint64_t initialLoadCost = sortedMachines[i].second;
		const Machine & machine = instance.machines()[m];
		std::vector<ProcessID> processes;
		for (ProcessID p = 0; p < pCount; ++p) {
			if (x.solution()[p] == m) {
				processes.push_back(p);
			}
		}
		uint64_t loadCost = initialLoadCost;
		while(loadCost > 0) {
			uint64_t initialObj = x.objective();
			uint64_t bestObj = initialObj;
			Move bestMove(0, 0, 0);
			for (MachineCount j = 0; j < lowLoadMachines.size(); ++j) {
				const MachineID m2 = lowLoadMachines[j];
				for (ProcessCount i = 0; i < processes.size(); ++i) {
					const ProcessID p = processes[i];
					Move move(p, m, m2);
					if (mv.feasible(move)) {
						uint64_t obj = mv.objective(move);
						if (obj < bestObj) {
							bestObj = obj;
							bestMove = move;
						}
					}
				}
			}
			if (bestObj < initialObj) {
				mv.objective(bestMove);
				mv.commit(bestMove);
				auto pItr = std::find(processes.begin(), processes.end(), bestMove.p());
				processes.erase(pItr);
				loadCost = evaluateLoadCost(machine, r, x.usage(m, r));
			} else {
				break;
			}
		}
		#if TRACE_LCOPT >= 2
		std::cout << "Load cost for resource " << r;
		std::cout << " due to machine " << m << " reduced from ";
		std::cout << initialLoadCost << " to " << loadCost << std::endl;
		#endif
	}
}

void LoadCostOptimizer::optimize(SolutionInfo & x) const {
	const Problem & instance = x.instance();
	const ResourceCount rCount = instance.resources().size();
	#if TRACE_LCOPT >= 1
	writeCostComposition(x);
	#endif
	std::vector<uint64_t> weightedResourceLC(rCount);
	std::vector<ResourceID> sortedResources(rCount);
	for (ResourceID r = 0; r < rCount; ++r) {
		uint64_t weight = instance.resources()[r].weightLoadCost();
		weightedResourceLC[r] = x.loadCost(r) * weight;
		sortedResources[r] = r;
	}
	std::sort(sortedResources.begin(),
			  sortedResources.end(),
			  VectorComparer<ResourceID,uint64_t,true>(weightedResourceLC));
	for (ResourceCount i = 0; i < rCount; ++i) {
		const ResourceID r = sortedResources[i];
		#if TRACE_LCOPT >= 1
		std::cout << "Optimize load cost of resource " << r << std::endl;
		#endif
		optimizeResource(x, r);
		#if TRACE_LCOPT >= 1
		uint64_t weight = instance.resources()[r].weightLoadCost();
		std::cout << "Weighted load cost reduced from " << weightedResourceLC[r];
		std::cout << " to " << x.loadCost(r) * weight << std::endl;
		std::cout << "Objective value: " << x.objective() << std::endl;
		#endif
	}
	for (ResourceID r = 0; r < rCount; ++r) {
		uint64_t weight = instance.resources()[r].weightLoadCost();
		weightedResourceLC[r] = x.loadCost(r) * weight;
		sortedResources[r] = r;
	}
	std::sort(sortedResources.begin(),
			  sortedResources.end(),
			  VectorComparer<ResourceID,uint64_t,true>(weightedResourceLC));
	for (ResourceCount i = 0; i < rCount; ++i) {
		std::cout << "Weighted load cost of resource ";
		std::cout << sortedResources[i] << ":\t";
		std::cout << weightedResourceLC[sortedResources[i]] << std::endl;
	}
	#if TRACE_LCOPT >= 1
	writeCostComposition(x);
	#endif
}
