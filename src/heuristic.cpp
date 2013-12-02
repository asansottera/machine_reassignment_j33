#include "heuristic.h"
#include <vector>
#include <utility>
#include <sstream>

using namespace R12;

void Heuristic::init(const Problem & instance, const std::vector<MachineID> & initial, const uint32_t seed, const AtomicFlag & flag, SolutionPool & pool, const std::string & config) {
	m_instancePtr = &instance;
	m_initialPtr = &initial;
	m_seed = seed;
	m_flagPtr = &flag;
	m_poolPtr = &pool;
	m_params.init(config);
}

void Heuristic::signalCompletion() {
	pthread_mutex_lock(&m_completedMutex);
	m_completed = true;
	pthread_cond_signal(&m_completedCond);
	pthread_mutex_unlock(&m_completedMutex);
}

void Heuristic::signalError(const std::string & message) {
	pthread_mutex_lock(&m_completedMutex);
	m_completed = true;
	m_error = true;
	m_errorMessage = message;
	pthread_cond_signal(&m_completedCond);
	pthread_mutex_unlock(&m_completedMutex);
}

void Heuristic::start(const timespec & deadline) {
	CHECK(!m_runningAsync);
	m_deadline = deadline;
	m_interrupted = false;
	m_completed = false;
	m_error = false;
	m_errorMessage = "";
	m_runningAsync = true;
	#ifdef _WIN32
	m_statTime.tv_sec = time(NULL);
	m_startTime.tv_nsec = 0;
	#else
	clock_gettime(CLOCK_REALTIME, &m_startTime);
	#endif
	m_totalTime = timeDifference(deadline, m_startTime);
	int err = pthread_create(&m_thread, 0, &Heuristic::doRun, this);
	if (err != 0) throw std::runtime_error("Heuristic: pthread_create failed");
}

void Heuristic::join() {
	int err = pthread_join(m_thread, 0);
	if (err != 0) throw std::runtime_error("Heuristic: pthread_join failed");
	m_runningAsync = false;
}

bool Heuristic::wait() {
	pthread_mutex_lock(&m_completedMutex);
	int err = 0;
	while (!m_completed && err == 0) {
		err = pthread_cond_timedwait(&m_completedCond, &m_completedMutex, &m_deadline);
	}
	pthread_mutex_unlock(&m_completedMutex);
	return err == 0;
}

