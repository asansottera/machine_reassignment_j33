#include "exchange_verifier.h"

using namespace std;
using namespace R12;

bool ExchangeVerifier::feasible(const Exchange & exchange) const {
	if (exchange.p1() == exchange.p2()) {
		return true;
	}
	if (exchange.m1() == exchange.m2()) {
		return true;
	}
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const ServiceID s1 = process1.service();
	const ServiceID s2 = process2.service();
	const Service & service1 = instance().services()[s1];
	const Service & service2 = instance().services()[s2];
	const Machine & machine1 = instance().machines()[m1];
	const Machine & machine2 = instance().machines()[m2];
	const NeighborhoodID n1 = machine1.neighborhood();
	const NeighborhoodID n2 = machine2.neighborhood();
	const LocationID l1 = machine1.location();
	const LocationID l2 = machine2.location();
	// check capacity constraints
	const std::vector<ResourceID> & nonTransient = instance().nonTransientResources();
	for (auto ri = nonTransient.begin(); ri != nonTransient.end(); ++ri) {
		ResourceID r = *ri;
		uint32_t req1 = process1.requirement(r);
		uint32_t req2 = process2.requirement(r);
		uint32_t u1 = info().usage(m1,r) - req1 + req2;
		if (u1 > machine1.capacity(r)) {
			return false;
		}
		uint32_t u2 = info().usage(m2,r) - req2 + req1;
		if (u2 > machine2.capacity(r)) {
			return false;
		}
	}
	// check transient capacity constraints
	bool fromInitial1 = (m1 == initial()[p1]);
	bool fromInitial2 = (m2 == initial()[p2]);
	bool toInitial1 = (m2 == initial()[p1]);
	bool toInitial2 = (m1 == initial()[p2]);
	const std::vector<ResourceID> & transient = instance().transientResources();
	for (auto ri = transient.begin(); ri != transient.end(); ++ri) {
		ResourceID r = *ri;
		uint32_t req1 = process1.requirement(r);
		uint32_t req2 = process2.requirement(r);
		uint32_t tu1 = info().usage(m1,r) +
					   info().transient(m1,r) - 
					   (fromInitial1 ? 0 : req1) +
					   (toInitial2 ? 0 : req2);
		if (tu1 > machine1.capacity(r)) {
			return false;
		}
		uint32_t tu2 = info().usage(m2,r) +
					   info().transient(m2,r) -
					   (fromInitial2 ? 0 : req2) +
					   (toInitial1 ? 0 : req1);
		if (tu2 > machine2.capacity(r)) {
			return false;
		}
	}
	// exchange processes of the same service => no violation possible
	if (s1 == s2) {
		return true;
	}
	// check conflict and spread constraints for service 1
	if (!instance().serviceHasSingleProcess(s1)) {
		// check conflict
		if (info().boolMachinePresence(s1, m2)) {
			return false;
		}
		// check spread
		if (l1 != l2) {
			if (info().spread(s1) == service1.spreadMin() &&
				info().locationPresence(s1, l1) == 1 &&
				info().locationPresence(s1, l2) != 0) {
				return false;
			}
		}
	}
	// check conflict and spread constraints for service 2
	if (!instance().serviceHasSingleProcess(s2)) {
		// check conflict
		if (info().boolMachinePresence(s2, m1)) {
			return false;
		}
		// check spread
		if (l1 != l2) {
			if (info().spread(s1) == service1.spreadMin() &&
				info().locationPresence(s1, l1) == 1 &&
				info().locationPresence(s1, l2) != 0) {
				return false;
			}
			if (info().spread(s2) == service2.spreadMin() &&
				info().locationPresence(s2, l2) == 1 &&
				info().locationPresence(s2, l1) != 0) {
				return false;
			}
		}
	}
	// check dependency constraints
	// exchange on machines in the same neighborhood => no violation possible
	if (n1 != n2) {
		if (!checkDependency(s1, s2, n1, n2)) {
			return false;
		}
		if (!checkDependency(s2, s1, n2, n1)) {
			return false;
		}
	}
	// no constraint violated
	return true;
}

bool ExchangeVerifier::checkDependency(const ServiceID s,
									   const ServiceID x, 
									   const NeighborhoodID nsrc,
									   const NeighborhoodID ndst) const {
	if (!instance().serviceHasNoOutDependency(s)) {
		// check dependencies of s are satisfied in ndst
		auto out = boost::out_edges(s, instance().dependency());
		for (auto di = out.first; di != out.second; ++di) {
			const ServiceID s2 = boost::target(*di, instance().dependency());
			// if no processes of s2 are in ndst
			if (info().neighborhoodPresence(s2, ndst) == 0) {
				return false;
			}
			// if one process of s2 is in ndst but it is being moved to nsrc
			if (s2 == x && info().neighborhoodPresence(s2, ndst) == 1) {
				return false;
			}
		}
	}
	if (!instance().serviceHasNoInDependency(s)) {
		// if s will not be present in nsrc anymore, check that no dependency on s is broken
		if (info().neighborhoodPresence(s, nsrc) == 1) {
			auto in = boost::in_edges(s, instance().dependency());
			for (auto di = in.first; di != in.second; ++di) {
				const ServiceID s1 = boost::source(*di, instance().dependency());
				// if at least two processes of s1 are in nsrc
				if (info().neighborhoodPresence(s1, nsrc) > 1) {
					return false;
				}
				// if one process of s1 is in nsrc and it is being moved to ndst
				if (s1 != x && info().neighborhoodPresence(s1, nsrc) == 1) {
					return false;
				}
			}
		}
	}
	return true;
}

void ExchangeVerifier::computeDiffLoadCost(const Exchange & exchange) const {
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const Machine & machine1 = instance().machines()[m1];
	const Machine & machine2 = instance().machines()[m2];
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {
		uint32_t req1 = process1.requirement(r);
		uint32_t req2 = process2.requirement(r);
		uint32_t u1 = info().usage(m1, r);
		uint32_t u2 = info().usage(m2, r);
		uint32_t sc1 = machine1.safetyCapacity(r);
		uint32_t sc2 = machine2.safetyCapacity(r);
		// old load costs
		int64_t old_lc1 = _computeLoadCost(u1, sc1);
		int64_t old_lc2 = _computeLoadCost(u2, sc2);
		// new load costs
		int64_t new_lc1 = _computeLoadCost(u1 + req2 - req1, sc1);
		int64_t new_lc2 = _computeLoadCost(u2 + req1 - req2, sc2);
		// delta
		m_costDiff.loadCostDiff(r) = new_lc1 + new_lc2 - old_lc1 - old_lc2;
	}
}

void ExchangeVerifier::computeDiffBalanceCost(const Exchange & exchange) const {
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const Machine & machine1 = instance().machines()[m1];
	const Machine & machine2 = instance().machines()[m2];
	for (BalanceCostID b = 0; b < instance().balanceCosts().size(); ++b) {
		const BalanceCost & balance = instance().balanceCosts()[b];
		const ResourceID r1 = balance.resource1();
		const ResourceID r2 = balance.resource2();
		uint32_t req1r1 = process1.requirement(r1);
		uint32_t req1r2 = process1.requirement(r2);
		uint32_t req2r1 = process2.requirement(r1);
		uint32_t req2r2 = process2.requirement(r2);
		uint32_t u1r1 = info().usage(m1, r1);
		uint32_t u1r2 = info().usage(m1, r2);
		uint32_t u2r1 = info().usage(m2, r1);
		uint32_t u2r2 = info().usage(m2, r2);
		int64_t old_bc1 = _computeBalanceCost(balance, machine1, u1r1, u1r2);
		int64_t old_bc2 = _computeBalanceCost(balance, machine2, u2r1, u2r2);
		int64_t new_bc1 = _computeBalanceCost(balance, machine1,
											  u1r1 + req2r1 - req1r1,
											  u1r2 + req2r2 - req1r2);
		int64_t new_bc2 = _computeBalanceCost(balance, machine2,
											  u2r1 + req1r1 - req2r1,
											  u2r2 + req1r2 - req2r2);
		m_costDiff.balanceCostDiff(b) = new_bc1 + new_bc2 - old_bc1 - old_bc2;
	}
}

void ExchangeVerifier::computeDiffProcessMoveCost(const Exchange & exchange) const {
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const MachineID mi1 = initial()[p1];
	const MachineID mi2 = initial()[p2];
	if (m1 == mi1) {
		m_costDiff.processMoveCostDiff() += process1.movementCost();
	} else if (m2 == mi1) {
		m_costDiff.processMoveCostDiff() -= process1.movementCost();
	}
	if (m2 == mi2) {
		m_costDiff.processMoveCostDiff() += process2.movementCost();
	} else if (m1 == mi2) {
		m_costDiff.processMoveCostDiff() -= process2.movementCost();
	}
}

void ExchangeVerifier::computeDiffServiceMoveCost(const Exchange & exchange) const {
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const ServiceID s1 = process1.service();
	const ServiceID s2 = process2.service();
	const MachineID mi1 = initial()[p1];
	const MachineID mi2 = initial()[p2];
	if (s1 == s2) {
		if (info().movedProcesses(s1) + 2u > info().serviceMoveCost()) {
			// service move cost might have changed
			int64_t delta = 0;
			if (m1 == mi1) {
				// p1 moved away from initial
				++delta;
			}
			if (m2 == mi2) {
				// p2 moved away from initial
				++delta;
			}
			if (m2 == mi1) {
				// p1 moved back to initial
				--delta;
			}
			if (m1 == mi2) {
				// p2 moved back to initial
				--delta;
			}
			if (delta > 0) {
				uint64_t moved = info().movedProcesses(s1) + delta;
				if (moved > info().serviceMoveCost()) {
					// service move cost increased (+1 or +2)
					m_costDiff.serviceMoveCostDiff() = static_cast<int64_t>(moved) -
													   static_cast<int64_t>(info().serviceMoveCost());
				}
			}
			if (delta < 0 && info().movedProcesses(s1) == info().serviceMoveCost()) {
				// service move cost might have decreased
				uint64_t max = info().movedProcesses(s1) + delta;
				for (ServiceID s = 0; s < instance().services().size(); ++s) {
					if (s != s1 && info().movedProcesses(s) > max) {
						// another service becomes the one with the most moved processes
						max = info().movedProcesses(s);
					}
					// at most, the service move cost remains the same
					if (max == info().serviceMoveCost()) {
						break;
					}
				}
				// update service move cost difference
				m_costDiff.serviceMoveCostDiff() = static_cast<int64_t>(max) -
												   static_cast<int64_t>(info().serviceMoveCost());
			}
		}
	} else {
		int64_t delta1 = 0;
		int64_t delta2 = 0;
		if (m1 == mi1) {
			++delta1;
		}
		if (m2 == mi2) {
			++delta2;
		}
		if (m2 == mi1) {
			--delta1;
		}
		if (m1 == mi2) {
			--delta2;
		}
		bool increased = false;
		if (delta1 > 0 || delta2 > 0) {
			uint64_t moved1 = info().movedProcesses(s1) + delta1;
			uint64_t moved2 = info().movedProcesses(s2) + delta2;
			uint64_t max = std::max(moved1, moved2);
			if (max > info().serviceMoveCost()) {
				// at most, increased by 1
				m_costDiff.serviceMoveCostDiff() = 1;
				increased = true;
			}
		}
		if (!increased) {
			// service move cost decreases or stays the same
			if ((delta1 < 0 && info().movedProcesses(s1) == info().serviceMoveCost()) ||
				(delta2 < 0 && info().movedProcesses(s2) == info().serviceMoveCost())) {
				uint64_t max = 0;
				for (ServiceID s = 0; s < instance().services().size(); ++s) {
					int64_t delta = 0;
					if (s == s1) delta = delta1;
					if (s == s2) delta = delta2;
					uint64_t moved = info().movedProcesses(s) + delta;
					if (moved > max) {
						max = moved;
					}
					if (max == info().serviceMoveCost()) {
						// at most, the service move cost remains the same
						break;
					}
				}
				// decrease or mantain service move cost
				m_costDiff.serviceMoveCostDiff() = static_cast<int64_t>(max) -
												   static_cast<int64_t>(info().serviceMoveCost());
			}
		}
	}
}

void ExchangeVerifier::computeDiffMachineMoveCost(const Exchange & exchange) const {
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const MachineID im1 = initial()[p1];
	const MachineID im2 = initial()[p2];
	m_costDiff.machineMoveCostDiff() += instance().machineMoveCost(im1, m2);
	m_costDiff.machineMoveCostDiff() -= instance().machineMoveCost(im1, m1);
	m_costDiff.machineMoveCostDiff() += instance().machineMoveCost(im2, m1);
	m_costDiff.machineMoveCostDiff() -= instance().machineMoveCost(im2, m2);
}

uint64_t ExchangeVerifier::objective(const Exchange & exchange) const {
	if (exchange.p1() == exchange.p2()) {
		return info().objective();
	}
	if (exchange.m1() == exchange.m2()) {
		return info().objective();
	}
	m_costDiff.reset();
	computeDiffLoadCost(exchange);
	computeDiffBalanceCost(exchange);
	computeDiffProcessMoveCost(exchange);
	computeDiffServiceMoveCost(exchange);
	computeDiffMachineMoveCost(exchange);	
	return m_costDiff.objective();
}

void ExchangeVerifier::commit(const Exchange & exchange) {
	CHECK(exchange.p1() != exchange.p2());
	if (exchange.m1() == exchange.m2()) {
		return;
	}
	const ProcessID p1 = exchange.p1();
	const ProcessID p2 = exchange.p2();
	const MachineID m1 = exchange.m1();
	const MachineID m2 = exchange.m2();
	const Process & process1 = instance().processes()[p1];
	const Process & process2 = instance().processes()[p2];
	const Machine & machine1 = instance().machines()[m1];
	const Machine & machine2 = instance().machines()[m2];
	const MachineID im1 = initial()[p1];
	const MachineID im2 = initial()[p2];
	// exchange processes
	info().solution()[p1] = m2;
	info().solution()[p2] = m1;
	// update usage and transient usage
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {
		const Resource & resource = instance().resources()[r];
		uint32_t req1 = process1.requirement(r);
		uint32_t req2 = process2.requirement(r);
		uint32_t u1 = info().usage(m1, r);
		uint32_t u2 = info().usage(m2, r);
		info().setUsage(m1, r, u1 - req1 + req2);
		info().setUsage(m2, r, u2 - req2 + req1);
		if (resource.transient()) {
			int32_t delta1 = 0;
			int32_t delta2 = 0;
			if (m1 == im1) {
				// move process 1 away from initial machine m1
				delta1 += req1;
			}
			if (m2 == im2) {
				// move process 2 away from initial machine m2
				delta2 += req2;
			}
			if (m2 == im1) {
				// move process 1 back to initial machine m2
				delta2 -= req1;
			}
			if (m1 == im2) {
				// move process 2 back to initial machine m1
				delta1 -= req2;
			}
			// avoid memory access if delta1 == 0
			if (delta1 != 0) {
				uint32_t tu1 = info().transient(m1, r);
				info().setTransient(m1, r, tu1 + delta1);
			}
			// avoid memory access if delta2 == 0
			if (delta2 != 0) {
				uint32_t tu2 = info().transient(m2, r);
				info().setTransient(m2, r, tu2 + delta2);
			}
		}
	}
	// update presence
	const ServiceID s1 = process1.service();
	const ServiceID s2 = process2.service();
	if (s1 != s2) {
		info().setBoolMachinePresence(s1, m1, false);
		info().setBoolMachinePresence(s1, m2, true);
		info().setBoolMachinePresence(s2, m1, true);
		info().setBoolMachinePresence(s2, m2, false);
		info().setMachinePresence(s1, m1, info().machinePresence(s1, m1) - 1);
		info().setMachinePresence(s1, m2, info().machinePresence(s1, m2) + 1);
		info().setMachinePresence(s2, m1, info().machinePresence(s2, m1) + 1);
		info().setMachinePresence(s2, m2, info().machinePresence(s2, m2) - 1);
		const LocationID l1 = machine1.location();
		const LocationID l2 = machine2.location();
		if (l1 != l2) {
			info().setLocationPresence(s1, l1, info().locationPresence(s1, l1) - 1);
			info().setLocationPresence(s1, l2, info().locationPresence(s1, l2) + 1);
			info().setLocationPresence(s2, l1, info().locationPresence(s2, l1) + 1);
			info().setLocationPresence(s2, l2, info().locationPresence(s2, l2) - 1);
		}
		const NeighborhoodID n1 = machine1.neighborhood();
		const NeighborhoodID n2 = machine2.neighborhood();
		if (n1 != n2) {
			info().setNeighborhoodPresence(s1, n1, info().neighborhoodPresence(s1, n1) - 1);
			info().setNeighborhoodPresence(s1, n2, info().neighborhoodPresence(s1, n2) + 1);
			info().setNeighborhoodPresence(s2, n1, info().neighborhoodPresence(s2, n1) + 1);
			info().setNeighborhoodPresence(s2, n2, info().neighborhoodPresence(s2, n2) - 1);
		}
	}
	// update number of moved processes
	int32_t deltaMoved1 = 0;
	if (m1 == im1) {
		deltaMoved1 = +1;
	} else if (m2 == im1) {
		deltaMoved1 = -1;
	}
	info().setMovedProcesses(s1, info().movedProcesses(s1) + deltaMoved1);
	uint32_t deltaMoved2 = 0;
	if (m2 == im2) {
		deltaMoved2 = +1;
	} else if (m1 == im2) {
		deltaMoved2 = -1;
	}
	info().setMovedProcesses(s2, info().movedProcesses(s2) + deltaMoved2);
	// update costs
	m_costDiff.apply();
}
