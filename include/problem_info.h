#ifndef R12_PROBLEM_INFO_H
#define R12_PROBLEM_INFO_H

#include "common.h"
#include "problem.h"
#include <utility>
#include <vector>
#include <boost/cstdint.hpp>
#include <iostream>

namespace R12 {

class Statistics {
public:
	float min;
	float max;
	float mean;
};

inline std::ostream & operator<<(std::ostream & out, const Statistics & stat) {
	out << stat.min << "|" << stat.max << "|" << stat.mean;
	return out;
}

class ProblemInfo {
private:
	typedef std::pair<ProcessID,float> PReq;
	typedef std::pair<MachineID,float> MCap;
private:
	const Problem & m_instance;
	const std::vector<MachineID> & m_initial;
	/*! Total aggregate requirement */
	float m_req;
	/*! Total aggregate capacity */
	float m_cap;
	/*! Total aggregate safety capacity */
	float m_safetyCap;
	/*! Total resource requirement */
	std::vector<float> m_rReq;
	/*! Total resource capacity */
	std::vector<float> m_rCap;
	/*! Total resource safety capacity */
	std::vector<float> m_rSafetyCap;
	/*! Per-process aggregate resource requirement */
	std::vector<float> m_pReq;
	/*! Per-machine aggregate capacity */
	std::vector<float> m_mCap;
	/*! Per-machine aggregate safety capacity */
	std::vector<float> m_mSafetyCap;
	/*! Processes sorted by aggregate requirement */
	std::vector<PReq> m_p_by_pReq;
	/*! Machines sorted by aggregate capacity */
	std::vector<MCap> m_m_by_mCap;
	/*! Machines sorted by aggregate safety capacity */
	std::vector<MCap> m_m_by_mSafetyCap;
	float m_fReq10;
	float m_fReq05;
	float m_fReq01;
	float m_fCap10;
	float m_fCap05;
	float m_fCap01;
	float m_fSafetyCap10;
	float m_fSafetyCap05;
	float m_fSafetyCap01;
	/*! Count services */
	ServiceCount m_singleProcServices;
	ServiceCount m_multiProcServices;
	ServiceCount m_noInDepServices;
	ServiceCount m_noOutDepServices;
	ServiceCount m_noDepServices;
	/*! Statistics */
	Statistics m_pPerServiceStats;
	Statistics m_inDepStats;
	Statistics m_outDepStats;
	Statistics m_mPerLocationStats;
	Statistics m_mPerNeighborhoodStats;
private:
	class PReqComp {
	public:
		bool operator()(const PReq & pr1, const PReq & pr2) const {
			return pr1.second > pr2.second;
		}
	};
	class MCapComp {
	public:
		bool operator()(const MCap & mc1, const MCap & mc2) const {
			return mc1.second > mc2.second;
		}
	};
private:
	float computeAggregateReq(const ProcessID p) {
		const Process & process = m_instance.processes()[p];
		float res = 0;
		for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
			float req = process.requirement(r);
			res += req;
		}
		return res;
	}
	float computeAggregateCap(const MachineID m) {
		const Machine & machine = m_instance.machines()[m];
		float res = 0;
		for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
			float cap = machine.capacity(r);
			res += cap;
		}
		return res;
	}
	float computeAggregateSafetyCap(const MachineID m) {
		const Machine & machine = m_instance.machines()[m];
		float res = 0;
		for (ResourceID r = 0; r < m_instance.resources().size(); ++r) {
			float cap = machine.safetyCapacity(r);
			res += cap;
		}
		return res;
	}
	float totalResourceReqFraction(const ResourceID r) const {
		return m_rReq[r] / m_req;
	}
	float totalResourceCapFraction(const ResourceID r) const {
		return m_rCap[r] / m_cap;
	}
	float totalResourceSafetyCapFraction(const ResourceID r) const {
		return m_rSafetyCap[r] / m_safetyCap;
	}
	float computeRequirementFraction(const float pFrac) const {
		float res = 0;
		ProcessCount pPartialCount = std::round(m_instance.processes().size() * pFrac);
		for (ProcessID p = 0; p < pPartialCount; ++p) {
			res += m_p_by_pReq[p].second;
		}
		res /= m_req;
		return res;
	}
	float computeCapacityFraction(const float mFrac) const {
		float res = 0;
		MachineCount mPartialCount = std::round(m_instance.machines().size() * mFrac);
		for (MachineID m = 0; m < mPartialCount; ++m) {
			res += m_m_by_mCap[m].second;
		}
		res /= m_cap;
		return res;
	}
	float computeSafetyCapacityFraction(const float mFrac) const {
		float res = 0;
		MachineCount mPartialCount = std::round(m_instance.machines().size() * mFrac);
		for (MachineID m = 0; m < mPartialCount; ++m) {
			res += m_m_by_mSafetyCap[m].second;
		}
		res /= m_safetyCap;
		return res;
	}
public:
	ProblemInfo(const Problem & instance, const std::vector<MachineID> & initial);
	float requirementFraction(const float pFrac) const {
		return computeRequirementFraction(pFrac);
	}
	float capacityFraction(const float mFrac) const {
		return computeCapacityFraction(mFrac);
	}
	float safetyCapacityFraction(const float mFrac) const {
		return computeSafetyCapacityFraction(mFrac);
	}
	Statistics processesPerServiceStats() const {
		return m_pPerServiceStats;
	}
	Statistics machinesPerLocationStats() const {
		return m_mPerLocationStats;
	}
	Statistics machinesPerNeighborhoodStats() const {
		return m_mPerNeighborhoodStats;
	}
	Statistics inDependenciesPerServiceStats() const {
		return m_inDepStats;
	}
	Statistics outDependenciesPerServiceStats() const {
		return m_outDepStats;
	}
	void report(std::ostream & out) const;
};

}

#endif
