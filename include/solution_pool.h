#ifndef R12_SOLUTION_POOL_H
#define R12_SOLUTION_POOL_H

#include "common.h"
#include "solution_delta.h"
#include <set>
#include <memory>
#include <vector>
#include <queue>
#include <utility>
#include <stdexcept>
#include <iostream>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <pthread.h>

namespace R12 {

class SolutionPool {
private:
	#if BOOST_VERSION < 104800
	typedef boost::uniform_int<std::size_t> UniformInt;
	#else
	typedef boost::random::uniform_int_distribution<std::size_t> UniformInt;
	#endif
public:
	typedef std::shared_ptr<const std::vector<MachineID>> SolutionPtr;
	class Entry {
	private:
		uint64_t m_obj;
		ProcessCount m_delta;
		SolutionPtr m_ptr;
	public:
		Entry() : m_obj(0), m_delta(0) {
		}
		Entry(uint64_t obj, ProcessCount delta, SolutionPtr ptr) : m_obj(obj), m_delta(delta), m_ptr(ptr) {
		}
		uint64_t obj() const {
			return m_obj;
		}
		ProcessCount delta() const {
			return m_delta;
		}
		const SolutionPtr & ptr() const {
			return m_ptr;
		}
	};
	struct ObjComp {
		bool operator()(const Entry & a, const Entry & b) const {
			return a.obj() < b.obj();
		}
	};
	struct DeltaComp {
		bool operator()(const Entry & a, const Entry & b) const {
			return a.delta() > b.delta();
		}
	};
	typedef uint32_t SubscriberID;
	class Subscription {
	private:
		pthread_mutex_t m_mutex;
		pthread_cond_t m_cond;
		std::queue<Entry> m_entries;
		bool m_shutdownEvent;
		Entry m_entry;
	public:
		Subscription() {
			pthread_mutex_init(&m_mutex, 0);
			pthread_cond_init(&m_cond, 0);
			m_shutdownEvent = false;
		}
		~Subscription() {
			pthread_mutex_destroy(&m_mutex);
			pthread_cond_destroy(&m_cond);
		}
		void enqueue(const Entry & entry) {
			pthread_mutex_lock(&m_mutex);
			m_entries.push(entry);
			pthread_cond_signal(&m_cond);
			pthread_mutex_unlock(&m_mutex);
		}
		bool trywait() {
			pthread_mutex_lock(&m_mutex);
			bool found = m_entries.size() > 0 && !m_shutdownEvent;
			if (found) {
				m_entry = m_entries.front();
				m_entries.pop();
			}
			pthread_mutex_unlock(&m_mutex);
			return found;
		}
		bool wait() {
			pthread_mutex_lock(&m_mutex);
			while (m_entries.size() == 0 && !m_shutdownEvent) {
				pthread_cond_wait(&m_cond, & m_mutex);
			}
			if (m_shutdownEvent) {
				pthread_mutex_unlock(&m_mutex);
				return false;
			} else {
				m_entry = m_entries.front();
				m_entries.pop();
				pthread_mutex_unlock(&m_mutex);
				return true;
			}
		}
		Entry dequeue() {
			return m_entry;
		}
		void shutdown() {
			pthread_mutex_lock(&m_mutex);
			m_shutdownEvent = true;
			pthread_cond_signal(&m_cond);
			pthread_mutex_unlock(&m_mutex);
		}
	};
	typedef std::shared_ptr<Subscription> SubscriptionPtr;
private:
	uint32_t m_hqMinBestDelta;
	double m_hdMaxBestObjRatio;
private:
	std::multiset<Entry, ObjComp> m_hqEntries;
	std::multiset<Entry, DeltaComp> m_hdEntries;
	mutable pthread_rwlock_t m_lock;
	std::size_t m_maxHighQuality;
	std::size_t m_maxHighDiversity;
	mutable pthread_rwlock_t m_subscriptionLock;
	std::vector<SubscriptionPtr> m_subscriptions;
	mutable boost::taus88 m_gen;
private:
	Entry makeEntry(uint64_t obj, const std::vector<MachineID> & solution) {
		SolutionPtr ptr(new std::vector<MachineID>(solution));
		if (m_hqEntries.size() == 0) {
			return Entry(obj, 0, ptr);
		} else {
			return Entry(obj, delta(*(m_hqEntries.begin()->ptr()), solution), ptr);
		}
	}
	bool pushHighQuality(const Entry & entry) {
		// insert among high quality solutions
		bool isHighQuality = false;
		if (m_maxHighQuality > 0) {
			// must be different from the current best
			if (m_hqEntries.size() == 0) {
				m_hqEntries.insert(entry);
				isHighQuality = true;
			} else if (entry.delta() >= m_hqMinBestDelta) {
				// keep if there is room or it is better than the worst
				if (m_hqEntries.size() < m_maxHighQuality) {
					m_hqEntries.insert(entry);
					isHighQuality = true;
				} else {
					auto worstItr = --(m_hqEntries.rbegin().base());
					if (entry.obj() < worstItr->obj()) {
						m_hqEntries.erase(worstItr);
						m_hqEntries.insert(entry);
						isHighQuality = true;
					}
				}
			}
		}
		return isHighQuality;
	}
	bool pushHighDiversity(const Entry & entry) {
		// insert among high diversity solutions
		bool isHighDiversity = false;
		if (m_maxHighDiversity > 0) {
			// must not be too bad compared to the current best
			if (m_hqEntries.size() > 0 && entry.obj() < m_hdMaxBestObjRatio * m_hqEntries.begin()->obj())  {
				if (m_hdEntries.size() < m_maxHighDiversity) {
					m_hdEntries.insert(entry);
					isHighDiversity = true;
				} else {
					auto worstItr = --(m_hdEntries.rbegin().base());
					if (entry.delta() >= worstItr->delta()) {
						m_hdEntries.erase(worstItr);
						m_hdEntries.insert(entry);
						isHighDiversity = true;
					}
				}
			}
		}
		return isHighDiversity;
	}
	std::pair<bool,bool> push(const Entry & entry) {
		bool isHighQuality = false;
		bool isHighDiversity = false;
		// if this is the new best
		if (m_hqEntries.size() > 0 && entry.obj() < m_hqEntries.begin()->obj()) {
			isHighQuality = true;
			isHighDiversity = false;
			std::vector<Entry> reinsertList;
			// extract high quality entries
			for (auto itr = m_hqEntries.begin(); itr != m_hqEntries.end(); ++itr) {
				reinsertList.push_back(Entry(itr->obj(), delta(*entry.ptr(), *itr->ptr()), itr->ptr()));
			}
			m_hqEntries.clear();
			// extract high diversity entries
			for (auto itr = m_hdEntries.begin(); itr != m_hdEntries.end(); ++itr) {
				reinsertList.push_back(Entry(itr->obj(), delta(*entry.ptr(), *itr->ptr()), itr->ptr()));
			}
			m_hdEntries.clear();
			// insert new best
			m_hqEntries.insert(entry);
			// push new entries
			for (auto itr = reinsertList.begin(); itr != reinsertList.end(); ++itr) {
				pushHighQuality(*itr);
				pushHighDiversity(*itr);
			}
		} else {
			isHighQuality = pushHighQuality(entry);
			isHighDiversity = pushHighDiversity(entry);
		}
		// notify subscribers
		if (isHighQuality || isHighDiversity) {
			pthread_rwlock_rdlock(&m_subscriptionLock);
			for (auto itr = m_subscriptions.begin(); itr != m_subscriptions.end(); ++itr) {
				SubscriptionPtr subPtr = *itr;
				subPtr->enqueue(entry);
			}
			pthread_rwlock_unlock(&m_subscriptionLock);
		}
		return std::pair<bool,bool>(isHighQuality, isHighDiversity);
	}
public:
	SolutionPool(std::size_t maxHighQuality, std::size_t maxHighDiversity, ProcessCount hqMinBestDelta, double hdMaxBestObjRatio) {
		int err;
		err = pthread_rwlock_init(&m_lock, 0);
		if (err != 0) {
			throw std::runtime_error("Unable to initialize read-write lock");
		}
		err = pthread_rwlock_init(&m_subscriptionLock, 0);
		if (err != 0) {
			pthread_rwlock_destroy(&m_lock);
			throw std::runtime_error("Unable to initialize read-write lock");
		}
		m_maxHighQuality = maxHighQuality;
		m_maxHighDiversity = maxHighDiversity;
		m_hqMinBestDelta = hqMinBestDelta;
		m_hdMaxBestObjRatio = hdMaxBestObjRatio;
	}
	~SolutionPool() {
		pthread_rwlock_destroy(&m_lock);
		pthread_rwlock_destroy(&m_subscriptionLock);
	}
	/*! Retrieves the best entry. Returns true on success. */
	bool best(Entry & entry) const {
		pthread_rwlock_rdlock(&m_lock);
		bool available = m_hqEntries.size() > 0;
		if (available) {
			entry = (*m_hqEntries.begin());
		}
		pthread_rwlock_unlock(&m_lock);
		return available;
	}
	/*! Retrieves the worst high-quality entry. Returns true on success. */
	bool worst(Entry & entry) const {
		pthread_rwlock_rdlock(&m_lock);
		bool available = m_hqEntries.size() > 0;
		if (available) {
			entry = (*m_hqEntries.rbegin());
		}
		pthread_rwlock_unlock(&m_lock);
		return available;
	}
	/*! Retrieves a randomly-chosen entry from the high-quality pool. Returns true on success. */
	bool randomHighQuality(Entry & entry) const {
		pthread_rwlock_rdlock(&m_lock);
		bool available = m_hqEntries.size() > 0;
		if (available) {
			UniformInt positionDist(0, m_hqEntries.size() - 1);
			std::size_t position = positionDist(m_gen);
			auto itr = m_hqEntries.begin();
			std::advance(itr, position);
			entry = *itr;
		}
		pthread_rwlock_unlock(&m_lock);
		return available;
	}
	/*! Retrieves a randomly-chosen entry from the high-diversity pool. Returns false on success. */
	bool randomHighDiversity(Entry & entry) const {
		pthread_rwlock_rdlock(&m_lock);
		bool available = m_hdEntries.size() > 0;
		if (available) {
			UniformInt positionDist(0, m_hdEntries.size() - 1);
			std::size_t position = positionDist(m_gen);
			auto itr = m_hdEntries.begin();
			std::advance(itr, position);
			entry = *itr;
		}
		pthread_rwlock_unlock(&m_lock);
		return available;
	}
	/*! Proposes a solution for insertion in the pool.
	    The first element of the pair is true if the solution was added to the high-quality pool.
	    The second element of the pair is true if the solution was added to the high-diversity pool. */
	std::pair<bool,bool> push(uint64_t obj, const std::vector<MachineID> & solution) {
		pthread_rwlock_wrlock(&m_lock);
		std::pair<bool,bool> result = push(makeEntry(obj, solution));
		pthread_rwlock_unlock(&m_lock);
		return result;
	}
	/*! Subsribes for notification of new solutions inserted in the pool. */
	SubscriptionPtr subscribe() {
		pthread_rwlock_wrlock(&m_subscriptionLock);
		SubscriptionPtr subPtr(new Subscription);
		m_subscriptions.push_back(subPtr);
		pthread_rwlock_unlock(&m_subscriptionLock);
		return subPtr;
	}
	/*! Unsubscribe for notification of new solutions inserted in the pool. */
	void unsubscribe(const SubscriptionPtr subPtr) {
		pthread_rwlock_wrlock(&m_subscriptionLock);
		std::remove(m_subscriptions.begin(), m_subscriptions.end(), subPtr);
		pthread_rwlock_unlock(&m_subscriptionLock);
	}
	/*! Interrupts all subscribers waiting for notification. */
	void shutdown() {
		pthread_rwlock_rdlock(&m_subscriptionLock);
		for (auto itr = m_subscriptions.begin(); itr != m_subscriptions.end(); ++itr) {
			SubscriptionPtr subPtr = *itr;
			subPtr->shutdown();
		}
		pthread_rwlock_unlock(&m_subscriptionLock);
	}
};

}

#endif
