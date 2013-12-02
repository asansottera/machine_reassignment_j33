#include "move_verifier.h"

using namespace R12;

inline void MoveVerifier::computeDiffLoadCost(const Move & move) const
{
	const ProcessID p = move.p();
	const MachineID src = move.src();
	const MachineID dst = move.dst();
	const Process & process = instance().processes()[p];
	const Machine & srcMachine = instance().machines()[src];
	const Machine & dstMachine = instance().machines()[dst];
	for (ResourceID r = 0; r < instance().resources().size(); ++r) {	
		uint32_t req = process.requirement(r);
		uint32_t u_src = info().usage(src, r);
		uint32_t u_dst = info().usage(dst, r);
		uint32_t sc_src = srcMachine.safetyCapacity(r);
		uint32_t sc_dst = dstMachine.safetyCapacity(r);
		// old load costs
		int64_t old_lc_src = _computeLoadCost(u_src, sc_src);
		int64_t old_lc_dst = _computeLoadCost(u_dst, sc_dst);
		// new load costs
		int64_t new_lc_src = _computeLoadCost(u_src - req, sc_src);
		int64_t new_lc_dst = _computeLoadCost(u_dst + req, sc_dst);
		// compute load cost difference
		DiffLoadCost[r] = new_lc_src + new_lc_dst - old_lc_src - old_lc_dst;
	}
}

inline void MoveVerifier::computeDiffBalanceCost(const Move & move) const {

	const Process & process = m_info.instance().processes()[move.p()];
	const Machine & srcMachine = m_info.instance().machines()[move.src()];
	const Machine & dstMachine = m_info.instance().machines()[move.dst()];

	for (BalanceCostID b = 0; b < m_info.instance().balanceCosts().size(); ++b) {
			const BalanceCost & balance = m_info.instance().balanceCosts()[b];
			// Compute difference balance cost for BalanceCost b on src machine
			int64_t SrcOldBalCost = R12::_computeBalanceCost(balance, srcMachine, m_info.usage(move.src(), balance.resource1()),m_info.usage(move.src(), balance.resource2()));
			int64_t SrcNewBalCost = R12::_computeBalanceCost(balance, srcMachine, m_info.usage(move.src(), balance.resource1())-process.requirement(balance.resource1()),m_info.usage(move.src(), balance.resource2())-process.requirement(balance.resource2()));
			DiffBalCost[b] = SrcNewBalCost - SrcOldBalCost;

			// Compute difference balance cost for BalanceCost b on dst machine
			int64_t DstOldBalCost = R12::_computeBalanceCost(balance, dstMachine, m_info.usage(move.dst(), balance.resource1()),m_info.usage(move.dst(), balance.resource2()));
			int64_t DstNewBalCost = R12::_computeBalanceCost(balance, dstMachine, m_info.usage(move.dst(), balance.resource1())+process.requirement(balance.resource1()),m_info.usage(move.dst(), balance.resource2())+process.requirement(balance.resource2()));
			DiffBalCost[b] += DstNewBalCost - DstOldBalCost;
	}
}
	
inline void MoveVerifier::computeDiffProcessMoveCost(const Move & move) const {
	const Process & process = m_info.instance().processes()[move.p()];

	DiffProcessMoveCost = 0;

	// src Machine is always different from dst Machine
	if (m_info.initial()[move.p()] == move.src()) 
		DiffProcessMoveCost = process.movementCost();

	if (m_info.initial()[move.p()] == move.dst()) 
		DiffProcessMoveCost -= process.movementCost();
}

	
inline void MoveVerifier::computeDiffServiceMoveCost(const Move & move) const {
	const Process & process = m_info.instance().processes()[move.p()];
	const ServiceID s = process.service();

	DiffServiceMoveCost = 0;

	if (m_info.initial()[move.p()] == move.src())
	{
		// The service s of p had the max number of moved processes: the new service move cost increase of 1
		if(m_info.movedProcesses(s) == m_info.serviceMoveCost())
			DiffServiceMoveCost = 1;
	}
	
	if (m_info.initial()[move.p()] == move.dst()) 
	{
		if(m_info.movedProcesses(s) ==m_info.serviceMoveCost())
		{
			ServiceID srv=0;
			for(;srv<m_info.instance().services().size();srv++)
				if(srv!=s)
					if(m_info.movedProcesses(srv) == m_info.serviceMoveCost())
						break;
			// If it is true, the number of moved processes of service s is strictly greater than 
			// the number of moved processes of the other services
			if(srv==m_info.instance().services().size())
				DiffServiceMoveCost = -1;
		}
	}		
}
	
inline void MoveVerifier::computeDiffMachineMoveCost(const Move & move) const {
	const MachineID im = m_info.initial()[move.p()];
	DiffMachineMoveCost = instance().machineMoveCost(im, move.dst());
	DiffMachineMoveCost -= instance().machineMoveCost(im, move.src());
}

uint64_t MoveVerifier::computeObjective(const Move & move) const {

	if (move.src() == move.dst()) {
		return info().objective();
	}

	// Recompute difference costs
	computeDiffBalanceCost(move);
	computeDiffLoadCost(move);
	computeDiffMachineMoveCost(move);
	computeDiffProcessMoveCost(move);
	computeDiffServiceMoveCost(move);

	uint64_t totalLoadCost = 0;
	for (ResourceID r = 0; r < m_info.instance().resources().size(); ++r) {
		const Resource & resource = m_info.instance().resources()[r];
		totalLoadCost += resource.weightLoadCost() * (DiffLoadCost[r] + m_info.loadCost(r));
	}
	uint64_t totalBalanceCost = 0;
	for (BalanceCostID b = 0; b < m_info.instance().balanceCosts().size(); ++b) {
		const BalanceCost & balance = m_info.instance().balanceCosts()[b];
		totalBalanceCost += balance.weight() * (DiffBalCost[b]+ m_info.balanceCost(b));
	}
	uint64_t totalProcessMoveCost = m_info.instance().weightProcessMoveCost() * (DiffProcessMoveCost+m_info.processMoveCost());
	uint64_t totalServiceMoveCost = m_info.instance().weightServiceMoveCost() * (DiffServiceMoveCost+m_info.serviceMoveCost());
	uint64_t totalMachineMoveCost= m_info.instance().weightMachineMoveCost() * (DiffMachineMoveCost+m_info.machineMoveCost());
	uint64_t obj =
		totalLoadCost +
		totalBalanceCost +
		totalProcessMoveCost +
		totalServiceMoveCost +
		totalMachineMoveCost;
	return obj;
}

void MoveVerifier::commit(const Move & move) {
	ProcessID p = move.p();
	MachineID src = move.src();
	MachineID dst = move.dst();
	if (src != dst) {
		// change solution 
		m_info.solution()[p] = dst;

		// get data
		const Process & process = m_info.instance().processes()[p];
		const Machine & srcMachine = m_info.instance().machines()[src];
		const Machine & dstMachine = m_info.instance().machines()[dst];

		// update balance costs
		for (BalanceCostID b = 0; b < m_info.instance().balanceCosts().size(); ++b) 
			m_info.setBalanceCost(b,m_info.balanceCost(b)+DiffBalCost[b]);

		for (ResourceID r = 0; r < m_info.instance().resources().size(); ++r) {
			// update usage
			m_info.setUsage(src, r,
				m_info.usage(src, r) - process.requirement(r));
			m_info.setUsage(dst, r,
				m_info.usage(dst, r) + process.requirement(r));
			// update transient
			if (m_info.instance().resources()[r].transient()) {
				if (m_info.initial()[p] == src) {
					m_info.setTransient(m_info.initial()[p], r,
						m_info.transient(m_info.initial()[p], r) + process.requirement(r));
				}
				if (m_info.initial()[p] == dst) {
					m_info.setTransient(m_info.initial()[p], r,
						m_info.transient(m_info.initial()[p], r) - process.requirement(r));
				}
			}
			// update load cost
			m_info.setLoadCost(r,m_info.loadCost(r)+DiffLoadCost[r]);
		}
		// update presence
		m_info.setBoolMachinePresence(process.service(), src, false);
		m_info.setBoolMachinePresence(process.service(), dst, true);
		m_info.setMachinePresence(process.service(), src,
			m_info.machinePresence(process.service(), src) - 1);
		m_info.setMachinePresence(process.service(), dst,
			m_info.machinePresence(process.service(), dst) + 1);
		m_info.setLocationPresence(process.service(), srcMachine.location(),
			m_info.locationPresence(process.service(), srcMachine.location()) - 1);
		m_info.setLocationPresence(process.service(), dstMachine.location(),
			m_info.locationPresence(process.service(), dstMachine.location()) + 1);
		m_info.setNeighborhoodPresence(process.service(), srcMachine.neighborhood(),
			m_info.neighborhoodPresence(process.service(), srcMachine.neighborhood()) - 1);
		m_info.setNeighborhoodPresence(process.service(), dstMachine.neighborhood(),
			m_info.neighborhoodPresence(process.service(), dstMachine.neighborhood()) + 1);

		// update movement costs
		m_info.setProcessMoveCost(m_info.processMoveCost() + DiffProcessMoveCost);

		if (m_info.initial()[p] == src) 
			m_info.setMovedProcesses(process.service(),m_info.movedProcesses(process.service()) + 1);

		if (m_info.initial()[p] == dst) 
			m_info.setMovedProcesses(process.service(),m_info.movedProcesses(process.service()) - 1);

		m_info.setMachineMoveCost(m_info.machineMoveCost() + DiffMachineMoveCost);	

		m_info.setServiceMoveCost(m_info.serviceMoveCost() + DiffServiceMoveCost);
	}
}

bool MoveVerifier::feasible(const Move & move) const {
	if (move.src() == move.dst()) {
		return true;
	}
	const ProcessID p = move.p();
	const MachineID src = move.src();
	const MachineID dst = move.dst();
	const ServiceID s = instance().processes()[p].service();
	const Process & process = instance().processes()[p];
	const Machine & srcMachine = instance().machines()[src];
	const Machine & dstMachine = instance().machines()[dst];
	const Service & service = instance().services()[s];
	// check capacity constraints
	for (auto rItr = instance().nonTransientResources().begin(); rItr != instance().nonTransientResources().end(); ++rItr) {
		ResourceID r = *rItr;
		if (info().usage(dst, r) + process.requirement(r) > dstMachine.capacity(r)) {
			return false;
		}
	}
	// check transient constraints
	bool backToInitial = dst == initial()[p];
	for (auto rItr = instance().transientResources().begin(); rItr != instance().transientResources().end(); ++rItr) {
		ResourceID r = *rItr;
		uint32_t delta = backToInitial ? 0 : process.requirement(r);
		if (info().usage(dst, r) + info().transient(dst, r) + delta > dstMachine.capacity(r)) {
			return false;
		}
	}
	if (!instance().serviceHasSingleProcess(s)) {
		// check conflict constraints
		if (info().boolMachinePresence(s, dst)) {
			return false;
		}
		// check spread violations
		const LocationID lsrc = srcMachine.location();
		const LocationID ldst = dstMachine.location();
		if (lsrc != ldst &&
			service.spreadMin() == info().spread(s) &&
			info().locationPresence(s, lsrc) == 1 &&
			info().locationPresence(s, ldst) != 0)  {
			return false;
		}
	}
	// check dependencies and reverse dependencies
	const NeighborhoodID nsrc = srcMachine.neighborhood();
	const NeighborhoodID ndst = dstMachine.neighborhood();
	if (nsrc != ndst) {
		if (!instance().serviceHasNoOutDependency(s)) {
			auto outDep = boost::out_edges(s, instance().dependency());
			for (auto dItr = outDep.first; dItr != outDep.second; ++dItr) {
				const ServiceID s2 = boost::target(*dItr, instance().dependency());
				if (info().neighborhoodPresence(s2, ndst) == 0) {
					return false;
				}
			}
		}
		if (!instance().serviceHasNoInDependency(s)) {
			if (info().neighborhoodPresence(s, nsrc) == 1) {
				auto inDep = boost::in_edges(s, instance().dependency());
				for (auto dItr = inDep.first; dItr != inDep.second; ++dItr) {
					const ServiceID s1 = boost::source(*dItr, instance().dependency());
					if (info().neighborhoodPresence(s1, nsrc) > 0) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

