#include "smart_shaker.h"
#include "verifier.h"
#include <vector>

using namespace R12;

SmartShaker::SmartShaker(uint32_t globalTrials, uint32_t maxInfeasibleSteps) {
	m_instancePtr = 0;
	m_initialPtr = 0;
	m_terminationFlagPtr = 0;
	m_globalTrials = globalTrials;
	m_globalMaxInfeasibleSteps = maxInfeasibleSteps;
}

void SmartShaker::init(const Problem & instance, const std::vector<MachineID> & initial, uint32_t seed, const AtomicFlag & terminationFlag) {
	// initialize data
	m_instancePtr = &instance;
	m_initialPtr = &initial;
	m_terminationFlagPtr = &terminationFlag;
	// initialize generator and probability distributions
	m_gen.seed(seed);
	m_pdist = boost::uniform_int<ProcessID>(0, instance.processes().size() - 1);
	m_pdistByService.clear();
	for (ServiceID s = 0; s < instance.services().size(); ++s) {
		ProcessCount pCount = instance.processesByService(s).size();
		m_pdistByService.push_back(boost::uniform_int<ServiceID>(0, pCount - 1));
	}
	m_mdist = boost::uniform_int<MachineID>(0, instance.machines().size() - 1);
	m_mdistByNeighborhood.clear();
	for (NeighborhoodID n = 0; n < instance.neighborhoodCount(); ++n) {
		MachineCount mCount = instance.machinesByNeighborhood(n).size();
		m_mdistByNeighborhood.push_back(boost::uniform_int<MachineID>(0, mCount - 1));
	}
	m_mdistByLocation.clear();
	for (LocationID l = 0; l < instance.locationCount(); ++l) {
		MachineCount mCount = instance.machinesByLocation(l).size();
		m_mdistByLocation.push_back(boost::uniform_int<LocationID>(0, mCount - 1));
	}
	m_ldist = boost::uniform_int<LocationID>(0, instance.locationCount() - 1);
	// initialize statistics
	m_failureCount = 0;
	m_shakes = 0;
}

inline Move SmartShaker::pickRandomMove() {
	if (m_realdist(m_gen) < 0.5) {
		Move move = advisor().advise();
		if (info().solution()[move.p()] == move.src()) {
			return move;
		}
	}
	ProcessID p = m_pdist(m_gen);
	MachineID src = info().solution()[p];
	MachineID dst;
	do {
		dst = m_mdist(m_gen);
	} while (dst == src);
	return Move(p, src, dst);
}

Move SmartShaker::pickRepairCapacityMove(const MachineID src) {
	CHECK(m_processesByMachine[src].size() > 0);
	// pick process from that machine
	auto itr = m_processesByMachine[src].begin();
	if (m_processesByMachine[src].size() > 1) {
		std::advance(itr, m_pdist(m_gen, m_processesByMachine[src].size() - 1));
	}
	ProcessID p = *itr;
	// pick another machine
	MachineID dst = m_mdist(m_gen);
	while (dst == src) {
		dst = m_mdist(m_gen);
	}
	return Move(p, src, dst);
}

Move SmartShaker::pickRepairSpreadMove(const ServiceID s) {
	// pick process of that service
	ProcessID p = instance().processesByService(s)[m_pdistByService[s](m_gen)];
	// pick location where the service is absent
	LocationID l = m_ldist(m_gen);
	while (info().locationPresence(s, l) > 0) {
		l = m_ldist(m_gen);
	}
	// pick machine from that location
	MachineID dst = instance().machinesByLocation(l)[m_mdistByLocation[l](m_gen)];
	// return move
	return Move(p, info().solution()[p], dst);
}

Move SmartShaker::pickRepairConflictMove(const ConflictViolation & cv) {
	// pick process of that service on that machine
	CHECK(info().machinePresence(cv.s(), cv.m()) > 0);
	const std::vector<ProcessID> & processes = instance().processesByService(cv.s());
	std::vector<ProcessID> candidates;
	for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
		MachineID src = info().solution()[*itr];
		if (cv.m() == src) {
			candidates.push_back(*itr);
		}
	}
	// clearly, this should true, otherwise there would be no conflict
	CHECK(candidates.size() > 1);
	// pick one of those processes
	std::size_t pIndex = m_pdistByService[cv.s()](m_gen, candidates.size() - 1);
	ProcessID p = candidates[pIndex];
	MachineID src = info().solution()[p];
	// pick a different machine as the destination
	MachineID dst = m_mdist(m_gen);
	while (dst == src)  {
		dst = m_mdist(m_gen);
	}
	return Move(p, src, dst);
}

Move SmartShaker::pickRepairDependencyMove(const DependencyViolation & dv) {
	ServiceID s2 = dv.s2();
	NeighborhoodID n = dv.n();
	// pick process of s2 and bring it to neighborhood n
	ProcessID p = instance().processesByService(s2)[m_pdistByService[s2](m_gen)];
	MachineID src = info().solution()[p];
	// pick machine in neighborhood n
	MachineID dst = instance().machinesByNeighborhood(n)[m_mdistByNeighborhood[n](m_gen)];
	return Move(p, src, dst);
}

inline Move SmartShaker::pickRepairMove() {
	// there might be different types of violations
	// for now, we just the first that we find
	// the important thing is to move toward the feasible region!
	if (bv().capacityViolations().size() > 0) {
		const MachineID & src = *bv().capacityViolations().begin();
		return pickRepairCapacityMove(src);
	} else if (bv().transientViolations().size() > 0) {
		const MachineID & src = *bv().transientViolations().begin();
		return pickRepairCapacityMove(src);
	} else if (bv().spreadViolations().size() > 0) {
		const ServiceID & s = *bv().spreadViolations().begin();
		return pickRepairSpreadMove(s);
	} else if (bv().conflictViolations().size() > 0) {
		const ConflictViolation & cv = *bv().conflictViolations().begin();
		return pickRepairConflictMove(cv);
	} else if (bv().dependencyViolations().size() > 0) {
		const DependencyViolation & dv = *bv().dependencyViolations().begin();
		return pickRepairDependencyMove(dv);
	} else {
		return pickRandomMove();
	}
}

void SmartShaker::updateProcessesByMachine(const Move & move) {
	std::set<ProcessID> & srcProcesses = m_processesByMachine[move.src()];
	std::set<ProcessID> & dstProcesses = m_processesByMachine[move.dst()];
	dstProcesses.insert(move.p());
	srcProcesses.erase(move.p());
}

inline bool SmartShaker::tryGlobalShake(const uint32_t k) {
	std::vector<Move> moves;
	uint32_t infeasibleSteps = 0;
	for (uint32_t i = 1; i <= k; ++i) {
		if (bv().feasible()) {
			Move move = pickRandomMove();
			bv().update(move);
			updateProcessesByMachine(move);
			moves.push_back(move);
		} else {
			++infeasibleSteps;
			if (infeasibleSteps > m_globalMaxInfeasibleSteps) {
				// too many steps in unfeasible region, give up
				break;
			} else if (infeasibleSteps == m_globalMaxInfeasibleSteps) {
				// last step in unfeasible region, try to repair
				for (uint32_t j = 0; j < m_globalTrials; ++j) {
					Move move = pickRepairMove();
					bv().update(move);
					if (!bv().feasible()) {
						bv().rollback(move);
					} else {
						// repaired!
						updateProcessesByMachine(move);
						moves.push_back(move);
						break;
					}
				}
			} else {
				// try to move toward feasible region
				Move move = pickRepairMove();
				bv().update(move);
				updateProcessesByMachine(move);
				moves.push_back(move);
			}
			// repaired, reset steps in uneasible
			if (bv().feasible()) {
				infeasibleSteps = 0;
			}
		}
	}
	if (!bv().feasible()) {
		bv().rollback(moves);
		for (auto ritr = moves.rbegin(); ritr != moves.rend(); ++ritr) {
			updateProcessesByMachine(ritr->reverse());
		}
		return false;
	} else {
		return true;
	}
}

void SmartShaker::shake(const uint32_t k, SolutionInfo & _info) {
	// initialize shake
	m_infoPtr = &_info;
	m_processesByMachine.resize(instance().machines().size());
	for (MachineID m = 0; m < instance().machines().size(); ++m) {
		m_processesByMachine[m].clear();
	}
	for (ProcessID p = 0; p < instance().processes().size(); ++p) {
		MachineID m = info().solution()[p];
		m_processesByMachine[m].insert(p);
	}	
	m_advisorPtr.reset(new GreedyAdvisor(info(), m_processesByMachine));
	m_bvPtr.reset(new BatchVerifier(info()));
	// loop until a feasible move sequence has been found
	bool found = false;
	while (!found && !terminationFlag().read()) {
		// pick move to infeasible random variable
		found = tryGlobalShake(k);
		if (!found) ++m_failureCount;
	}
	if (found) ++m_shakes;
	// release
	m_advisorPtr.reset();
	m_bvPtr.reset();
}
