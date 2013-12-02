#ifndef R12_EXCHANGE_VERIFIER_H
#define R12_EXCHANGE_VERIFIER_H

#include <boost/cstdint.hpp>
#include <vector>
#include "solution_info.h"
#include "exchange.h"
#include "cost_diff.h"

namespace R12 {

class ExchangeVerifier 
{
private:
	SolutionInfo & m_info;
	mutable CostDiff m_costDiff;
private:
	void computeDiffLoadCost(const Exchange & exchange) const;
	void computeDiffBalanceCost(const Exchange & exchange) const;
	void computeDiffProcessMoveCost(const Exchange & exchange) const;
	void computeDiffServiceMoveCost(const Exchange & exchange) const;
	void computeDiffMachineMoveCost(const Exchange & exchange) const;
	/*! Checks dependencies from/to service s when moved from nsrc to ndst
		while service x is moved from ndst to nsrc. */
	bool checkDependency(const ServiceID s, const ServiceID x, const NeighborhoodID nsrc, const NeighborhoodID ndst) const;
private:
	const Problem & instance() const { return m_info.instance(); }
	const std::vector<MachineID> & initial() const { return m_info.initial(); }
	SolutionInfo & info() { return m_info; }
public:
	ExchangeVerifier(SolutionInfo & info) : m_info(info), m_costDiff(info) {
	}
	const SolutionInfo & info() const { return m_info; }
	/*! Checks whether applying the given exchange leads to a feasible solution (assumes the current solution is feasible). */
	bool feasible(const Exchange & exchange) const;
	/*! Commits the new exchange, updating the SolutionInfo object. */
	void commit(const Exchange & exchange);
	/*! Computes the objective function for a single move. */
	uint64_t objective(const Exchange & exchange) const;
};

}

#endif
