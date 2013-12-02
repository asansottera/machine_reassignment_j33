#include "batch_verifier.h"

using namespace R12;

BatchVerifier::BatchVerifier(SolutionInfo & info)
	: m_info(info) {
	m_feasibleCached = false;
	m_objectiveCached = false;
}

void BatchVerifier::update(const std::vector<Move> & moves) {
	for (auto itr = moves.begin(); itr != moves.end(); ++itr) {
		const Move & move = *itr;
		update(move);
	}
}

void BatchVerifier::update(const Move & move) {
	CHECK(solution()[move.p()] == move.src());
	ProcessID p = move.p();
	MachineID src = move.src();
	MachineID dst = move.dst();
	if (src != dst) {
		// change solution and invalidate cache
		info().solution()[p] = dst;
		m_feasibleCached = false;
		m_objectiveCached = false;
		// get dataa
		const MachineID im = initial()[p];
		const Process & process = instance().processes()[p];
		const Machine & srcMachine = instance().machines()[src];
		const Machine & dstMachine = instance().machines()[dst];
		// keep track of things modified
		m_movesToCheck.push_back(move);
		// update balance costs
		for (BalanceCostID b = 0; b < instance().balanceCosts().size(); ++b) {
			const BalanceCost & balance = instance().balanceCosts()[b];
			info().setBalanceCost(b,
				info().balanceCost(b)
				- _computeBalanceCost(balance, srcMachine,
					info().usage(src, balance.resource1()),
					info().usage(src, balance.resource2()))
				+ _computeBalanceCost(balance, srcMachine,
					info().usage(src, balance.resource1()) - process.requirement(balance.resource1()),
					info().usage(src, balance.resource2()) - process.requirement(balance.resource2()))
				- _computeBalanceCost(balance, dstMachine,
					info().usage(dst, balance.resource1()),
					info().usage(dst, balance.resource2()))
				+ _computeBalanceCost(balance, dstMachine,
					info().usage(dst, balance.resource1()) + process.requirement(balance.resource1()),
					info().usage(dst, balance.resource2()) + process.requirement(balance.resource2()))
			);
		}
		for (ResourceID r = 0; r < instance().resources().size(); ++r) {
			// update usage
			uint32_t oldSrcUsage = info().usage(src, r);
			uint32_t oldDstUsage = info().usage(dst, r);
			info().setUsage(src, r,
				info().usage(src, r) - process.requirement(r));
			info().setUsage(dst, r,
				info().usage(dst, r) + process.requirement(r));
			// update transient
			if (instance().resources()[r].transient()) {
				if (initial()[p] == src) {
					info().setTransient(initial()[p], r,
						info().transient(initial()[p], r) + process.requirement(r));
				}
				if (initial()[p] == dst) {
					info().setTransient(initial()[p], r,
						info().transient(initial()[p], r) - process.requirement(r));
				}
			}
			// update load cost
			info().setLoadCost(r,
				info().loadCost(r)
				- _computeLoadCost(oldSrcUsage, srcMachine.safetyCapacity(r))
				+ _computeLoadCost(info().usage(src, r), srcMachine.safetyCapacity(r))
				- _computeLoadCost(oldDstUsage, dstMachine.safetyCapacity(r))
				+ _computeLoadCost(info().usage(dst, r), dstMachine.safetyCapacity(r)));
		}
		// update presence
		info().setMachinePresence(process.service(), src,
			info().machinePresence(process.service(), src) - 1);
		info().setMachinePresence(process.service(), dst,
			info().machinePresence(process.service(), dst) + 1);
		info().setLocationPresence(process.service(), srcMachine.location(),
			info().locationPresence(process.service(), srcMachine.location()) - 1);
		info().setLocationPresence(process.service(), dstMachine.location(),
			info().locationPresence(process.service(), dstMachine.location()) + 1);
		info().setNeighborhoodPresence(process.service(), srcMachine.neighborhood(),
			info().neighborhoodPresence(process.service(), srcMachine.neighborhood()) - 1);
		info().setNeighborhoodPresence(process.service(), dstMachine.neighborhood(),
			info().neighborhoodPresence(process.service(), dstMachine.neighborhood()) + 1);
		// update movement costs
		if (initial()[p] == src) {
			info().setProcessMoveCost(info().processMoveCost() + process.movementCost());
			info().setMovedProcesses(process.service(),
				info().movedProcesses(process.service()) + 1);
		}
		if (initial()[p] == dst) {
			info().setProcessMoveCost(info().processMoveCost() - process.movementCost());
			info().setMovedProcesses(process.service(),
				info().movedProcesses(process.service()) - 1);
		}
		if (initial()[p] == src || initial()[p] == dst) {
			info().computeServiceMoveCost();
		}
		info().setMachineMoveCost(
			info().machineMoveCost()
			- instance().machineMoveCost(im, src)
			+ instance().machineMoveCost(im, dst));
	}
}

void BatchVerifier::rollback(const std::vector<Move> & moves) {
	for (auto itr = moves.rbegin(); itr != moves.rend(); ++itr) {
		const Move & move = *itr;
		rollback(move);
	}
}

void BatchVerifier::rollback(const Move & move) {
	update(move.reverse());
}

inline bool BatchVerifier::checkCapacity(MachineID m) const {
	const Machine & machine = instance().machines()[m];
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {
		if (info().usage(m, r) > machine.capacity(r)) {
			return false;
		}
	}
	return true;
}

inline bool BatchVerifier::checkTransient(MachineID m) const {
	const Machine & machine = instance().machines()[m];
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {
		if (instance().resources()[r].transient()) {
			if (info().usage(m, r) + info().transient(m, r) > machine.capacity(r)) {
				return false;
			}
		}
	}
	return true;
}

bool BatchVerifier::checkFeasible() const {
	// list of recent moves is used to restrict the number of constraints to check
	for (auto itr = m_movesToCheck.begin(); itr != m_movesToCheck.end(); ++itr) {
		const Move & move = *itr;
		const ProcessID p = move.p();
		const MachineID src = move.src();
		const MachineID dst = move.dst();
		const ServiceID s = instance().processes()[p].service();
		const Machine & srcMachine = instance().machines()[src];
		const Machine & dstMachine = instance().machines()[dst];
		const Service & service = instance().services()[s];
		// check capacity constraints
		if (!checkCapacity(dst)) {
			m_capacityViolations.insert(dst);
		}
		if (checkCapacity(src)) {
			m_capacityViolations.erase(src);
		}
		// check transient constraints
		if (!checkTransient(dst)) {
			m_transientViolations.insert(dst);
		}
		if (checkTransient(src)) {
			m_transientViolations.erase(src);
		}
		// check conflict constraints
		if (info().machinePresence(s, dst) > 1) {
			m_conflictViolations.insert(ConflictViolation(s, dst));
		}
		if (info().machinePresence(s, src) <= 1) {
			m_conflictViolations.erase(ConflictViolation(s, src));
		}
		// check spread violations
		if (info().spread(s) < service.spreadMin()) {
			m_spreadViolations.insert(s);
		} else {
			m_spreadViolations.erase(s);
		}
		// check dependencies and reverse dependencies
		const NeighborhoodID nsrc = srcMachine.neighborhood();
		const NeighborhoodID ndst = dstMachine.neighborhood();
		if (nsrc != ndst) {
			auto outDep = boost::out_edges(s, instance().dependency());
			for (auto dItr = outDep.first; dItr != outDep.second; ++dItr) {
				const ServiceID s2 = boost::target(*dItr, instance().dependency());
				if (info().neighborhoodPresence(s2, ndst) == 0) {
					m_dependencyViolations.insert(DependencyViolation(s, s2, ndst));
				}
				if (info().neighborhoodPresence(s, nsrc) == 0) {
					m_dependencyViolations.erase(DependencyViolation(s, s2, nsrc));
				}
			}
			auto inDep = boost::in_edges(s, instance().dependency());
			for (auto dItr = inDep.first; dItr != inDep.second; ++dItr) {
				const ServiceID s1 = boost::source(*dItr, instance().dependency());
				if (info().neighborhoodPresence(s1, nsrc) > 0 && info().neighborhoodPresence(s, nsrc) == 0) {
					m_dependencyViolations.insert(DependencyViolation(s1, s, nsrc));
				}
				m_dependencyViolations.erase(DependencyViolation(s1, s, ndst));
			}
		}
	}
	m_movesToCheck.clear();
	return 	m_capacityViolations.empty() &&
		m_transientViolations.empty() &&
		m_conflictViolations.empty() &&
		m_spreadViolations.empty() &&
		m_dependencyViolations.empty();
}

uint64_t BatchVerifier::computeObjective() const {
	uint64_t totalLoadCost = 0;
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {
		const Resource & resource = instance().resources()[r];
		totalLoadCost += resource.weightLoadCost() * info().loadCost(r);
	}
	uint64_t totalBalanceCost = 0;
	for (BalanceCostID b = 0; b < instance().balanceCosts().size(); ++b) {
		const BalanceCost & balance = instance().balanceCosts()[b];
		totalBalanceCost += balance.weight() * info().balanceCost(b);
	}
	uint64_t totalProcessMoveCost = instance().weightProcessMoveCost() * info().processMoveCost();
	uint64_t totalServiceMoveCost = instance().weightServiceMoveCost() * info().serviceMoveCost();
	uint64_t totalMachineMoveCost = instance().weightMachineMoveCost() * info().machineMoveCost();
	uint64_t obj =
		totalLoadCost +
		totalBalanceCost +
		totalProcessMoveCost +
		totalServiceMoveCost +
		totalMachineMoveCost;
	return obj;
}

