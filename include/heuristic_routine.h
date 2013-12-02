#ifndef R12_HEURISTIC_ROUTINE_H
#define R12_HEURISTIC_ROUTINE_H

#include "common.h"
#include "problem.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include <boost/cstdint.hpp>
#include <boost/random.hpp>

namespace R12 {

class HeuristicRoutine {
private: // shared objects passed during initialization
	const Problem * m_instancePtr;
	const std::vector<MachineID> * m_initialPtr;
	const AtomicFlag * m_flagPtr;
	boost::mt19937 * m_rngPtr;
private: // routine state
	mutable bool m_interrupted;
protected:
	const Problem & instance() const {
		return *m_instancePtr;
	}
	const std::vector<MachineID> & initial() const {
		return *m_initialPtr;
	}
	bool interrupted() const {
		if (!m_interrupted) {
			m_interrupted = m_flagPtr->read();
		}
		return m_interrupted;
	}
	boost::mt19937 & rng() {
		return *m_rngPtr;
	}
public:
	void init(const Problem & instance,
			  const std::vector<MachineID> & initial,
			  const AtomicFlag & flag,
			  boost::mt19937 & rng,
			  const ParameterMap & parameters) {
		m_instancePtr = &instance;
		m_initialPtr = &initial;
		m_flagPtr = &flag;
		m_interrupted = false;
		m_rngPtr = &rng;
		configure(parameters);
	}
protected:
	virtual void configure(const ParameterMap & parameters) = 0;
public:
	virtual ~HeuristicRoutine() {
	}
};

}

#endif
