#ifndef R12_COST_DIFF_H
#define R12_COST_DIFF_H

#include "common.h"
#include "solution_info.h"
#include <boost/cstdint.hpp>
#include <vector>
#include <algorithm>

namespace R12 {

class CostDiff {
private:
	SolutionInfo & m_info;
	std::vector<int64_t> m_loadCostDiff;
	std::vector<int64_t> m_balanceCostDiff;
	int64_t m_processMoveCostDiff;
	int64_t m_serviceMoveCostDiff;
	int64_t m_machineMoveCostDiff;
public:
	CostDiff(SolutionInfo & info) : m_info(info),
									m_loadCostDiff(info.instance().resources().size()),
									m_balanceCostDiff(info.instance().balanceCosts().size()),
									m_processMoveCostDiff(0),
									m_serviceMoveCostDiff(0),
									m_machineMoveCostDiff(0) {
	}
	int64_t & loadCostDiff(const ResourceID r) {
		return m_loadCostDiff[r];
	}
	int64_t & balanceCostDiff(const BalanceCostID b) {
		return m_balanceCostDiff[b];
	}
	int64_t & processMoveCostDiff() {
		return m_processMoveCostDiff;
	}
	int64_t & serviceMoveCostDiff() {
		return m_serviceMoveCostDiff;
	}
	int64_t & machineMoveCostDiff() {
		return m_machineMoveCostDiff;
	}
	int64_t loadCostDiff(const ResourceID r) const {
		return m_loadCostDiff[r];
	}
	int64_t balanceCostDiff(const BalanceCostID b) const {
		return m_balanceCostDiff[b];
	}
	int64_t processMoveCostDiff() const {
		return m_processMoveCostDiff;
	}
	int64_t serviceMoveCostDiff() const {
		return m_serviceMoveCostDiff;
	}
	int64_t machineMoveCostDiff() const {
		return m_machineMoveCostDiff;
	}
	uint64_t objective() const {
		uint64_t obj = 0;
		for (ResourceID r = 0; r < m_info.instance().resources().size(); ++r) {
			const Resource & resource = m_info.instance().resources()[r];
			const uint64_t cost = m_info.loadCost(r) + m_loadCostDiff[r];
			obj += resource.weightLoadCost() * cost;
		}
		for (BalanceCostID b = 0; b < m_info.instance().balanceCosts().size(); ++b) {
			const BalanceCost & balance = m_info.instance().balanceCosts()[b];
			const uint64_t cost = m_info.balanceCost(b) + m_balanceCostDiff[b];
			obj += balance.weight() * cost;
		}
		obj += m_info.instance().weightProcessMoveCost() *
			   (m_info.processMoveCost() + m_processMoveCostDiff);
		obj += m_info.instance().weightServiceMoveCost() *
			   (m_info.serviceMoveCost() + m_serviceMoveCostDiff);
		obj += m_info.instance().weightMachineMoveCost() *
			   (m_info.machineMoveCost() + m_machineMoveCostDiff);
		return obj;
	}
	void apply() {
		const ResourceCount rCount = m_info.instance().resources().size();
		for (ResourceID r = 0; r < rCount; ++r) {
			m_info.setLoadCost(r, m_info.loadCost(r) + loadCostDiff(r));
		}
		const BalanceCostCount bCount = m_info.instance().balanceCosts().size();
		for (BalanceCostID b = 0; b < bCount; ++b) {
			m_info.setBalanceCost(b, m_info.balanceCost(b) + balanceCostDiff(b));
		}
		m_info.setProcessMoveCost(m_info.processMoveCost() + processMoveCostDiff());
		m_info.setServiceMoveCost(m_info.serviceMoveCost() + serviceMoveCostDiff());
		m_info.setMachineMoveCost(m_info.machineMoveCost() + machineMoveCostDiff());
	}
	void reset() {
		std::fill(m_loadCostDiff.begin(), m_loadCostDiff.end(), 0);
		std::fill(m_balanceCostDiff.begin(), m_balanceCostDiff.end(), 0);
		m_processMoveCostDiff = 0;
		m_serviceMoveCostDiff = 0;
		m_machineMoveCostDiff = 0;
	}
};

}

#endif
