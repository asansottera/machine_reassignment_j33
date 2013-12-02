#ifndef R12_MOVE_VERIFIER_H
#define R12_MOVE_VERIFIER_H

#include <boost/cstdint.hpp>
#include <vector>
#include "solution_info.h"
#include "move.h"

namespace R12 {

class MoveVerifier 
{
private:
	SolutionInfo & m_info;
	mutable std::vector<int64_t> DiffLoadCost;
	mutable std::vector<int64_t> DiffBalCost;
	mutable int64_t DiffProcessMoveCost;
	mutable int64_t DiffServiceMoveCost;
	mutable int64_t DiffMachineMoveCost;

	void computeDiffLoadCost(const Move & move) const;

	void computeDiffBalanceCost(const Move & move) const;

	void computeDiffProcessMoveCost(const Move & move) const;

	void computeDiffServiceMoveCost(const Move & move) const;

	void computeDiffMachineMoveCost(const Move & move) const;

	/*! Calculate new Objective cost for a single move. */
	uint64_t computeObjective(const Move & move) const;

private:
	const Problem & instance() const { return m_info.instance(); }
	const std::vector<MachineID> & initial() const { return m_info.initial(); }

public:
	MoveVerifier(SolutionInfo & info):m_info(info){
		DiffLoadCost.resize(info.instance().resources().size(),0);
		DiffBalCost.resize(info.instance().balanceCosts().size(),0);
		DiffProcessMoveCost=0;
		DiffMachineMoveCost=0;
		DiffServiceMoveCost=0;
	}

	const SolutionInfo & info() const { return m_info; }

	/*! Checks whether applying the given move leads to a feasible solution (assumes the current solution is feasible). */
	bool feasible(const Move & move) const;

	/*! Commit the new move to compute the new solution and update Info data structures . */
	void commit(const Move & move);

	/*! Computes the objective function for a single move. */
	uint64_t objective(const Move & move) const { return computeObjective(move); }

};

}

#endif
