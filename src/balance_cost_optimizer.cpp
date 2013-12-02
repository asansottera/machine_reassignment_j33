#include "balance_cost_optimizer.h"
#include "move_verifier.h"
#include "exchange_verifier.h"
#include "vector_comparer.h"
#include "pair_comparer.h"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace R12;

#define TRACE_BCOPT 1

std::vector<ProcessID> listProcesses(const MachineID m, const std::vector<MachineID> & x) {
	std::vector<ProcessID> processes;
	for (ProcessID p = 0; p < x.size(); ++p) {
		if (x[p] == m) {
			processes.push_back(p);
		}
	}
	return processes;
}

int64_t computeSignedBalanceCost(const BalanceCost & balance, const Machine & machine,
								 const uint32_t u1, const uint32_t u2) {
	uint32_t a1 = machine.capacity(balance.resource1()) - u1;
	uint32_t a2 = machine.capacity(balance.resource2()) - u2;
	int64_t cost = (int64_t)(balance.target() * a1) - (int64_t)(a2);
	return cost;
}

void BalanceCostOptimizer::optimizeBalance(SolutionInfo & x,
										   const BalanceCostID b) const {
	typedef std::pair<MachineID,int64_t> MBC;
	const Problem & instance = x.instance();
	const BalanceCost & balance = instance.balanceCosts()[b];
	const ResourceID r1 = balance.resource1();
	const ResourceID r2 = balance.resource2();
	const MachineCount mCount = instance.machines().size();
	std::vector<MBC> sortedPositive;
	std::vector<MBC> sortedNegative;
	int64_t psum = 0;
	int64_t sum = 0;
	for (MachineID m = 0; m < mCount; ++m) {
		const Machine & machine = instance.machines()[m];
		uint32_t u1 = x.usage(m, balance.resource1());
		uint32_t u2 = x.usage(m, balance.resource2());
		int64_t cost = computeSignedBalanceCost(balance, machine, u1, u2);
		if (cost > 0) {
			sortedPositive.push_back(MBC(m, cost));
			psum += cost;
		} else if (cost < 0) {
			sortedNegative.push_back(MBC(m, cost));
		}
		sum += cost;
	}
	#if TRACE_BCOPT >= 1
	std::cout << "Machines with positive balance cost " << b << ": ";
	std::cout << sortedPositive.size() << "/" << mCount << std::endl;
	std::cout << "Machines with negative balance cost " << b << ": ";
	std::cout << sortedNegative.size() << "/" << mCount << std::endl;
	std::cout << "Current balance cost " << b << ": " << psum << std::endl;
	std::cout << "Lower bound on balance cost " << b << ": " << sum << std::endl;
	#endif
	// We have two sets of machines:
	// - MP: machines with a positive balance cost b,
	//		 which are using too much r2 with respect to r1
	// - MN: machines with a negative balance cost b,
	//		 which can still increase their usage of r2 with respect to r1
	std::sort(sortedPositive.begin(),
			  sortedPositive.end(),
			  PairComparer<MachineID,int64_t,true>());
	std::sort(sortedNegative.begin(),
			  sortedNegative.end(),
			  PairComparer<MachineID,int64_t,false>());
	// We have two sets of processes:
	// - P1: processes with requirement of r1 greater than requirement of r2
	// - P2: processes with requirement of r2 greater than requirement of r1
	// We look for a pair of processes (p1,p2) such that:
	// - p1 in P2 such that M(p1) in MP
	// - p2 in P1 such that M(p2) in MN
	ExchangeVerifier ev(x);
	for (MachineCount i1 = 0; i1 < sortedPositive.size(); ++i1) {
		const MachineID m1 = sortedPositive[i1].first;
		const int64_t initialCost1 = sortedPositive[i1].second;
		const Machine machine1 = x.instance().machines()[m1];
		std::vector<ProcessID> p_of_m1 = listProcesses(m1, x.solution());
		int64_t cost1 = initialCost1;
		while (cost1 > 0) {
			uint64_t bestObj = x.objective();
			Exchange bestExchange(0, 0, 0, 0);
			for (MachineCount i2 = 0; i2 < sortedNegative.size(); ++i2) {
				const MachineID m2 = sortedNegative[i2].first;
				std::vector<ProcessID> p_of_m2 = listProcesses(m2, x.solution());
				for (ProcessCount j1 = 0; j1 < p_of_m1.size(); ++j1) {
					const ProcessID p1 = p_of_m1[j1];
					const Process & process1 = x.instance().processes()[p1];
					int64_t req11 = process1.requirement(r1);
					int64_t req12 = process1.requirement(r2);
					for (ProcessCount j2 = 0; j2 < p_of_m2.size(); ++j2) {
						const ProcessID p2 = p_of_m2[j2];
						const Process & process2 = x.instance().processes()[p2];
						int64_t req21 = process2.requirement(r1);
						int64_t req22 = process2.requirement(r2);
						int64_t delta1 = req11 - req21;
						int64_t delta2 = req12 - req22;
						if (balance.target() * delta1 - delta2 < 0) {
							// the exchange is feasible
							// because it reduces the balance cost
							// due to machine m1
							Exchange exchange(m1, p1, m2, p2);
							if (ev.feasible(exchange)) {
								uint64_t obj = ev.objective(exchange);
								if (obj < bestObj) {
									bestObj = obj;
									bestExchange = exchange;
								}
							}
						}
					}
				}
			}
			if (bestObj < x.objective()) {
				ev.objective(bestExchange);
				ev.commit(bestExchange);
				auto pItr = std::find(p_of_m1.begin(), p_of_m1.end(), bestExchange.p1());
				p_of_m1.erase(pItr);
				uint32_t u1 = x.usage(m1, r1);
				uint32_t u2 = x.usage(m1, r2);
				cost1 = computeSignedBalanceCost(balance, machine1, u1, u2);
			} else {
				break;
			}
		}
		#if TRACE_BCOPT >= 2
		std::cout << "Signed balance cost " << b << " due to machine " << m1;
		std::cout << " reduced from " << initialCost1 << " to " << cost1 << std::endl;
		#endif
	}
}

void BalanceCostOptimizer::optimize(SolutionInfo & x) const {
	const Problem & instance = x.instance();
	const BalanceCostCount bCount = instance.balanceCosts().size();
	#if TRACE_BCOPT >= 1
	writeCostComposition(x);
	#endif
	std::vector<uint64_t> weightedBalanceCosts(bCount);
	std::vector<BalanceCostID> sortedBalances(bCount);
	for (BalanceCostID b = 0; b < bCount; ++b) {
		uint64_t weight = instance.balanceCosts()[b].weight();
		weightedBalanceCosts[b] = x.balanceCost(b) * weight;
		sortedBalances[b] = b;
	}
	std::sort(sortedBalances.begin(),
			  sortedBalances.end(),
			  VectorComparer<BalanceCostID,uint64_t,true>(weightedBalanceCosts));
	for (BalanceCostCount i = 0; i < bCount; ++i) {
		const BalanceCostID b = sortedBalances[i];
		#if TRACE_BCOPT >= 1
		std::cout << "Optimize balance cost " << b << std::endl;
		#endif
		optimizeBalance(x, b);
		#if TRACE_BCOPT >= 1
		uint64_t weight = instance.balanceCosts()[b].weight();
		std::cout << "Weighted balance cost reduced from " << weightedBalanceCosts[b];
		std::cout << " to " << x.balanceCost(b) * weight << std::endl;
		std::cout << "Objective value: " << x.objective() << std::endl;
		#endif
	}
	#if TRACE_LCOPT >= 1
	writeCostComposition(x);
	#endif
}
