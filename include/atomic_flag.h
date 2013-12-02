#ifndef R12_ATOMIC_FLAG_H
#define R12_ATOMIC_FLAG_H

// Strategy 0: volatile
// Strategy 1: intrisics
// Strategy 2: std::atomic

#ifdef _WIN32
// on Windows volatile has acquire/release semantics
#define R12_ATOMIC_FLAG_STRATEGY 0
#else
// on Linux we default to intrisics which work both on g++ and icpc
#define R12_ATOMIC_FLAG_STRATEGY 1
#endif
// std::atomic is only supported by the intel compiler and causes incompatibilities with some boost headers

#if R12_ATOMIC_FLAG_STRATEGY == 2
#include <atomic>
#endif

namespace R12 {

class AtomicFlag {
private:
	#if R12_ATOMIC_FLAG_STRATEGY == 0
	volatile int32_t m_flag;
	#elif R12_ATOMIC_FLAG_STRATEGY == 1
	mutable int32_t m_flag;
	#elif R12_ATOMIC_FLAG_STRATEGY == 2
	std::atomic<int32_t> m_flag;
	#endif
public:
	AtomicFlag() {
		m_flag = 1;
		#if R12_ATOMIC_FLAG_STRATEGY == 1
		__sync_synchronize();
		#endif
	}
	bool read() const {
		bool value;
		#if R12_ATOMIC_FLAG_STRATEGY == 1
		__sync_synchronize();
		#endif
		value = (m_flag == 0);
		return value;
	}
	void write() {
		m_flag = 0;
		#if R12_ATOMIC_FLAG_STRATEGY == 1
		__sync_synchronize();
		#endif
	}
	void reset() {
		m_flag = 1;
		#if R12_ATOMIC_FLAG_STRATEGY == 1
		__sync_synchronize();
		#endif
	}
};

}

#endif
