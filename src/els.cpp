#include "els.h"
#include "batch_verifier.h"
#include <vector>
#include <algorithm>
#include <set>

// #define TRACE_ELS

#ifdef TRACE_ELS
#include <iostream>
#endif

using namespace R12;

inline int64_t ELS::distance(const SolutionInfo & info, const MachineID m) const {
	int64_t d = 0;
	const Machine & machine = info.instance().machines()[m];
	bool outside = false;
	for (ResourceID r = 0; r < info.instance().resources().size(); ++r) {
		const Resource & resource = info.instance().resources()[r];
		int64_t usage = static_cast<int64_t>(info.usage(m, r));
		int64_t sc = static_cast<int64_t>(machine.safetyCapacity(r));
		if (usage > sc) outside = true;
		d += resource.weightLoadCost() * std::abs(usage - sc);
	}
	if (outside) {
		d = -d;
	}
	return d;
}

inline int64_t ELS::distance(const SolutionInfo & info, const MachineID m, const ProcessID pOut, const ProcessID pIn) const {
	int64_t d = 0;
	const Machine & machine = info.instance().machines()[m];
	const Process & processOut = info.instance().processes()[pOut];
	const Process & processIn = info.instance().processes()[pIn];
	bool outside = false;
	for (ResourceID r = 0; r < info.instance().resources().size(); ++r) {
		const Resource & resource = info.instance().resources()[r];
		int64_t usage = static_cast<int64_t>(info.usage(m, r));
		usage -= processOut.requirement(r);
		usage += processIn.requirement(r);
		int64_t sc = static_cast<int64_t>(machine.safetyCapacity(r));
		if (usage > sc) outside = true;
		d += resource.weightLoadCost() * std::abs(usage - sc);
	}
	if (outside) {
		d = -d;
	}
	return d;
}

void ELS::runFromSolution(SolutionInfo & info) {
	m_bestObjective = info.objective();
	m_bestSolution = info.solution();
	// set seed
	srand(seed());
	// build vectors of machines below and above safety capacity
	std::vector<MachineID> mbsc;
	std::vector<MachineID> masc;
	for (MachineID m = 0; m < info.instance().machines().size(); ++m) {
		const Machine & machine = info.instance().machines()[m];
		bool above = false;
		for (ResourceID r = 0; r < info.instance().resources().size(); ++r) {
			if (info.usage(m, r) > machine.safetyCapacity(r)) {
				above = true;
				break;
			}
		}
		if (above) {
			masc.push_back(m);
		} else {
			mbsc.push_back(m);
		}
	}
	// initialize per-machine process set
	std::vector<std::set<ProcessID>> procByMachine(info.instance().machines().size());
	for (ProcessID p = 0; p < info.instance().processes().size(); ++p) {
		MachineID m = info.solution()[p];
		procByMachine[m].insert(p);
	}
	// initialize batch verifier
	BatchVerifier bv(info);
	// start local search
	bool continueLocalSearch = true;
	uint64_t it = 0;
	while(continueLocalSearch && !interrupted()) {
		++it;
		continueLocalSearch = false;
		// shuffle vectors
		std::random_shuffle(mbsc.begin(), mbsc.end());
		std::random_shuffle(masc.begin(), masc.end());
		for (auto bitr = mbsc.begin(); bitr != mbsc.end(); ++bitr) {
			for (auto aitr = masc.begin(); aitr != masc.end(); ++aitr) {
				MachineID m1 = *aitr;
				MachineID m2 = *bitr;
				int64_t m1Distance = distance(info, m1);
				int64_t m2Distance = distance(info, m2);
				for (auto p1Itr = procByMachine[m1].begin(); p1Itr != procByMachine[m1].end(); ++p1Itr) {
					for (auto p2Itr = procByMachine[m2].begin(); p2Itr != procByMachine[m2].end(); ++p2Itr) {
						ProcessID p1 = *p1Itr;
						ProcessID p2 = *p2Itr;
						int64_t m1NewDistance = distance(info, m1, p1, p2);
						int64_t m2NewDistance = distance(info, m2, p2, p1);
						if (std::abs(m1NewDistance) < std::abs(m1Distance) && std::abs(m2NewDistance) < std::abs(m2Distance)) {
							#ifdef TRACE_ELS
							std::cout << "ELS - Distance reduction exchanging " << p1 << "@" << m1 << " and " << p2 << "@" << m2 << std::endl;
							#endif
							// exchange processes
							Move move1(p1, m1, m2);
							Move move2(p2, m2, m1);
							bv.update(move1);
							bv.update(move2);
							if (bv.feasible() && bv.objective() < m_bestObjective) {
								#ifdef TRACE_ELS
								std::cout << "ELS - Applying exchange, objective " << bv.objective() << std::endl;
								#endif
								m_bestObjective = bv.objective();
								m_bestSolution = bv.solution();
								continueLocalSearch = true;
								procByMachine[m1].erase(p1Itr);
								procByMachine[m2].erase(p2Itr);
								procByMachine[m1].insert(p2);
								procByMachine[m2].insert(p1);
								if (m1NewDistance < 0) {
									std::remove(masc.begin(), masc.end(), m1);
									mbsc.push_back(m1);
								}
								if (m2NewDistance > 0) {
									std::remove(mbsc.begin(), mbsc.end(), m2);
									masc.push_back(m2);
								}
							} else {
								#ifdef TRACE_ELS
								std::cout << "ELS - Exchange is unfeasible" << std::endl;
								#endif
								bv.rollback(move1);
								bv.rollback(move2);
							}
						}
						if (continueLocalSearch) {
							break;
						}
					}
					if (continueLocalSearch || interrupted()) {
						break;
					}
				}
				if (continueLocalSearch || interrupted()) {
					break;
				}
			}
			if (continueLocalSearch || interrupted()) {
				break;
			}
		}
	}
}
