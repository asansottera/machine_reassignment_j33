#include "problem_info.h"
#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

using namespace R12;
using namespace boost::accumulators;

template<class T>
Statistics makeStatistics(const accumulator_set<T, stats< tag::min, tag::mean, tag::max > > & acc) {
	Statistics s;
	s.min = min(acc);
	s.max = max(acc);
	s.mean = mean(acc);
	return s;
}

ProblemInfo::ProblemInfo(const Problem & instance,
						 const std::vector<MachineID> & initial)
						 : m_instance(instance), m_initial(initial) {
	const ProcessCount pCount = instance.processes().size();
	const MachineCount mCount = instance.machines().size();
	const ResourceCount rCount = instance.resources().size();
	// resize vectors
	m_rReq.resize(rCount);
	m_rCap.resize(rCount);
	m_rSafetyCap.resize(rCount);
	m_pReq.resize(pCount);
	m_mCap.resize(mCount);
	m_mSafetyCap.resize(mCount);
	m_p_by_pReq.resize(pCount);
	m_m_by_mCap.resize(mCount);
	m_m_by_mSafetyCap.resize(mCount);
	// compute total resource requirements
	m_req = 0;
	for (ProcessID p = 0; p < pCount; ++p) {
		const Process & process = instance.processes()[p];
		for (ResourceID r = 0; r < rCount; ++r) {
			float prReq = process.requirement(r);
			m_req += prReq;
			m_rReq[r] += prReq;
		}
	}
	// compute total resource capacities and safety capacities
	m_cap = 0;
	m_safetyCap = 0;
	for (MachineID m = 0; m < mCount; ++m) {
		const Machine & machine = instance.machines()[m];
		for (ResourceID r = 0; r < rCount; ++r) {
			float mrCap = machine.capacity(r);
			m_cap += mrCap;
			m_rCap[r] += mrCap;
		}
		for (ResourceID r = 0; r < rCount; ++r) {
			float mrSafetyCap = machine.safetyCapacity(r);
			m_safetyCap += mrSafetyCap;
			m_rSafetyCap[r] += mrSafetyCap;
		}
	}
	// compute per-process aggregate requirement
	for (ProcessID p = 0; p < pCount; ++p) {
		float req = computeAggregateReq(p);
		m_pReq[p] = req;
		m_p_by_pReq[p].first = p;
		m_p_by_pReq[p].second = req;
	}
	// compute-per machine aggreagete capacity
	for (MachineID m = 0; m < mCount; ++m) {
		float cap = computeAggregateCap(m);
		m_mCap[m] = cap;
		m_m_by_mCap[m].first = m;
		m_m_by_mCap[m].second = cap;
		float safetyCap = computeAggregateSafetyCap(m);
		m_mSafetyCap[m] = safetyCap;
		m_m_by_mSafetyCap[m].first = m;
		m_m_by_mSafetyCap[m].second = safetyCap;
	}
	// sorted process and machines
	std::sort(m_p_by_pReq.begin(), m_p_by_pReq.end(), PReqComp());
	std::sort(m_m_by_mCap.begin(), m_m_by_mCap.end(), MCapComp());
	std::sort(m_m_by_mSafetyCap.begin(), m_m_by_mSafetyCap.end(), MCapComp());
	// fraction of total aggreagate requirement due to the largest processes
	m_fReq01 = computeRequirementFraction(0.01);
	m_fReq05 = computeRequirementFraction(0.05);
	m_fReq10 = computeRequirementFraction(0.10);
	// fraction of total aggregate capacity due to the largest machines
	m_fCap01 = computeCapacityFraction(0.01);
	m_fCap05 = computeCapacityFraction(0.05);
	m_fCap10 = computeCapacityFraction(0.10);
	// fraction of total aggregate safety capacity due to the largest machines
	m_fSafetyCap01 = computeSafetyCapacityFraction(0.01);
	m_fSafetyCap05 = computeSafetyCapacityFraction(0.05);
	m_fSafetyCap10 = computeSafetyCapacityFraction(0.10);
	// processes per service
	accumulator_set<ProcessCount, stats< tag::min, tag::mean, tag::max > > pPerServiceAcc;
	m_singleProcServices = 0;
	m_multiProcServices = 0;
	for (ServiceID s = 0; s < m_instance.services().size(); ++s) {
		ProcessCount pc = static_cast<ProcessCount>(m_instance.processesByService(s).size());
		pPerServiceAcc(pc);
		if (pc == 1) {
			++m_singleProcServices;
		} else {
			++m_multiProcServices;
		}
	}
	m_pPerServiceStats = makeStatistics(pPerServiceAcc);
	// dependencies and reverse dependencies per service
	accumulator_set<ServiceCount, stats< tag::min, tag::mean, tag::max > > inDepPerServiceAcc;
	accumulator_set<ServiceCount, stats< tag::min, tag::mean, tag::max > > outDepPerServiceAcc;
	m_noInDepServices = 0;
	m_noOutDepServices = 0;
	m_noDepServices = 0;
	for (ServiceID s = 0; s < m_instance.services().size(); ++s) {
		ServiceCount in = boost::in_degree(s, m_instance.dependency());
		ServiceCount out = boost::out_degree(s, m_instance.dependency());
		inDepPerServiceAcc(in);
		outDepPerServiceAcc(out);
		if (in == 0) {
			++m_noInDepServices;
		}
		if (out == 0) {
			++m_noOutDepServices;
		}
		if (in == 0 && out == 0) {
			++m_noDepServices;
		}
	}
	m_inDepStats = makeStatistics(inDepPerServiceAcc);
	m_outDepStats = makeStatistics(outDepPerServiceAcc);
	// machines per location
	accumulator_set<MachineCount, stats< tag::min, tag::mean, tag::max > > mPerLocationAcc;
	for (LocationID l = 0; l < m_instance.locationCount(); ++l) {
		MachineCount count = m_instance.machinesByLocation(l).size();
		mPerLocationAcc(count);
	}
	m_mPerLocationStats = makeStatistics(mPerLocationAcc);
	// machines per neighborhood
	accumulator_set<MachineCount, stats< tag::min, tag::mean, tag::max > > mPerNeighborhoodAcc;
	for (NeighborhoodID n = 0; n < m_instance.neighborhoodCount(); ++n) {
		MachineCount count = m_instance.machinesByNeighborhood(n).size();
		mPerNeighborhoodAcc(count);
	}
	m_mPerNeighborhoodStats = makeStatistics(mPerNeighborhoodAcc);
}

void ProblemInfo::report(std::ostream & out) const {
	for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
		out << "Resource " << r << " fraction of aggregate requirement: ";
		out << totalResourceReqFraction(r) << std::endl;
	}
	for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
		out << "Resource " << r << " ratio: ";
		out << m_rReq[r]/m_rCap[r] << std::endl;
	}
	for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
		out << "Resource " << r << " safety ratio: ";
		out <<  m_rReq[r]/m_rSafetyCap[r] << std::endl;
	}
	out << "Aggregate requirement of 1\% largest processes: ";
	out << m_fReq01 * 100 << "\%" << std::endl;
	out << "Aggregate requirement of 5\% largest processes: ";
	out << m_fReq05 * 100 << "\%" << std::endl;
	out << "Aggregate requirement of 10\% largest processes: ";
	out << m_fReq10 * 100 << "\%" << std::endl;
	out << "Aggregate capacity of 1\% largest machines: ";
	out << m_fCap01 * 100 << "\%" << std::endl;
	out << "Aggregate capacity of 5\% largest machines: ";
	out << m_fCap05 * 100 << "\%" << std::endl;
	out << "Aggregate capacity of 10\% largest machines: ";
	out << m_fCap10 * 100 << "\%" << std::endl;
	out << "Aggregate safety capacity of 1\% largest machines: ";
	out << m_fSafetyCap01 * 100 << "\%" << std::endl;
	out << "Aggregate safety capacity of 5\% largest machines: ";
	out << m_fSafetyCap05 * 100 << "\%" << std::endl;
	out << "Aggregate safety capacity of 10\% largest machines: ";
	out << m_fSafetyCap10 * 100 << "\%" << std::endl;
	// Count services
	out << "Services with a single process: ";
	out << m_singleProcServices << std::endl;
	out << "Services with multiple processes: ";
	out << m_multiProcServices << std::endl;
	out << "Services with no in-dependency: ";
	out << m_noInDepServices << std::endl;
	out << "Services with no out-dependency: ";
	out << m_noOutDepServices << std::endl;
	out << "Services with no dependency: ";
	out << m_noDepServices << std::endl;
	// Statistics
	out << "Processes per service [min|max|mean]: ";
	out << m_pPerServiceStats << std::endl;
	out << "In-dependencies per service [min|max|mean]: ";
	out << m_inDepStats << std::endl;
	out << "Out-dependencies per service [min|max|mean]: ";
	out << m_outDepStats << std::endl;
	out << "Machines per location [min|max|mean]: ";
	out << m_mPerLocationStats << std::endl;
	out << "Machines per neighborhood [min|max|mean]: ";
	out << m_mPerNeighborhoodStats << std::endl;
}
