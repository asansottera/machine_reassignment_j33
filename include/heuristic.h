#ifndef R12_HEURISTIC_H
#define R12_HEURISTIC_H

#include "problem.h"
#include "common.h"
#include "solution_info.h"
#include "solution_pool.h"
#include "atomic_flag.h"
#include "parameter_map.h"
#include <stdexcept>
#include <map>
#include <ctime>
#include <pthread.h>

namespace R12 {

/*! Abstract base class for heuristics that solve the ROADEF 2012 challenge. */
class Heuristic {
private:
	const Problem * m_instancePtr;
	const std::vector<MachineID> * m_initialPtr;
	const AtomicFlag * m_flagPtr;
	uint32_t m_seed;
	ParameterMap m_params;
	SolutionPool * m_poolPtr;
	bool m_completed;
	bool m_error;
	std::string m_errorMessage;
	timespec m_startTime;
	timespec m_deadline;
	double m_totalTime;
	mutable bool m_interrupted;
	pthread_t m_thread;
	pthread_mutex_t m_completedMutex;
	pthread_cond_t m_completedCond;
	bool m_runningAsync;
	static void * doRun(void * hPtr) {
		static_cast<Heuristic *>(hPtr)->run();
		return 0;
	}
protected:
	/*! Checks whether the heuristic should stop. */
	bool interrupted() const {
		if (!m_interrupted) {
			m_interrupted = flag().read();
		}
		return m_interrupted;
	}
	/*! Signals that the heuristic completed succesfully. */
	void signalCompletion();
	/*! Signals that the heuristic completed with an error. */
	void signalError(const std::string & message);
public:
	Heuristic() {
		m_instancePtr = 0;
		m_initialPtr = 0;
		m_poolPtr = 0;
		m_flagPtr = 0;
		pthread_mutex_init(&m_completedMutex, 0);
		pthread_cond_init(&m_completedCond, 0);
		m_runningAsync = false;
		m_interrupted = false;
		m_completed = false;
		m_error = false;
		m_errorMessage = "";
		m_startTime.tv_sec = 0;
		m_startTime.tv_nsec = 0;
	}
	virtual ~Heuristic() {
		pthread_mutex_destroy(&m_completedMutex);
		pthread_cond_destroy(&m_completedCond);
	}
private:
	double timeDifference(const timespec & t2, const timespec & t1) const {
		double sec = static_cast<double>(t2.tv_sec) - static_cast<double>(t1.tv_sec);
		double nsec = static_cast<double>(t2.tv_nsec) - static_cast<double>(t1.tv_nsec);
		return sec + nsec * 1e-9;
	}
protected:
	/*! Returns the problem instance. Do not call before initialization. */
	const Problem & instance() const {
		CHECK(m_instancePtr != 0);
		return *m_instancePtr;
	}
	/*! Returns the initial solution. Do not call before initialization. */
	const std::vector<MachineID> & initial() const {
		CHECK(m_initialPtr != 0);
		return *m_initialPtr;
	}
	/*! Returns the solution pool. Do not call before initialization. */
	SolutionPool & pool() {
		CHECK(m_poolPtr != 0);
		return *m_poolPtr;
	}
	const ParameterMap & parameters() const {
		return m_params;
	}
	uint32_t seed() const {
		return m_seed;
	}
	const AtomicFlag & flag() const {
		CHECK(m_flagPtr != 0);
		return *m_flagPtr;
	}
	bool hasParam(const std::string & key) const {
		return m_params.hasParam(key);
	}
	template<typename T> T param(const std::string & key) const {
		return m_params.param<T>(key);
	}
	template<typename T> T param(const std::string & key, const T def) const {
		return m_params.param<T>(key, def);
	}
	double elapsed() const {
		timespec now;
		#ifdef _WIN32
		now.tv_sec = time(NULL);
		now.tv_nsec = 0;
		#else
		clock_gettime(CLOCK_REALTIME, &now);
		#endif
		return timeDifference(now, m_startTime);
	}
	double remaining() const {
		timespec now;
		#ifdef _WIN32
		now.tv_sec = time(NULL);
		now.tv_nsec = 0;
		#else
		clock_gettime(CLOCK_REALTIME, &now);
		#endif
		return std::max(0.0, timeDifference(m_deadline, now));
	}
	double elapsedFraction() const {
		double tElapsed = elapsed();
		return std::min(1.0, tElapsed/m_totalTime);
	}
public:
	/*! Starts the heuristic in background, specifying the deadline before which the heuristic should terminate. */
	void start(const timespec & deadline);
	/*! Waits for the heuristic to stop. */
	void join();
	/*! Returns either when the deadline is expired or when the heuristic has completed.
		Returns true if the heuristic has completed, false otherwise. */
	bool wait();
	/*! Initializes the heuristic with problem instance data, the initial solution and the solution pool.
	    The objects are not copied: do not destroy them.  */
	void init(const Problem & instance, const std::vector<MachineID> & initial, const uint32_t seed, const AtomicFlag & flag, SolutionPool & pool, const std::string & config);
	/*! Runs the heuristic. */
	virtual void run() = 0;
	/*! Runs the heuristic starting from a given solution. */
	virtual void runFromSolution(SolutionInfo & info) {
		throw std::runtime_error("Partial execution not supported.");
	}
	/*! After the heuristic has stopped, returns the best solution found. */
	virtual const std::vector<MachineID> & bestSolution() const = 0;
	/*! After the heuristic has stopped, returns the objective value of the best solution found. */
	virtual uint64_t bestObjective() const = 0;
	/*! Returns true if an error occurred during the execution of the heuristic. */
	bool error() const {
		return m_error;
	}
	/*! Returns the error message for the error occurred during the execution of the heuristic. */
	std::string errorMessage() const {
		return m_errorMessage;
	}
};

}

#endif
