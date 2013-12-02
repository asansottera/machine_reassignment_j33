#ifndef R12_COMMON_H
#define R12_COMMON_H

#include <boost/cstdint.hpp>
#include <cassert>

typedef uint16_t MachineID;
typedef uint16_t MachineCount;
typedef uint16_t ResourceID;
typedef uint16_t ResourceCount;
typedef uint16_t ProcessID;
typedef uint16_t ProcessCount;
typedef uint16_t ServiceID;
typedef uint16_t ServiceCount;
typedef uint16_t NeighborhoodID;
typedef uint16_t NeighborhoodCount;
typedef uint16_t DependencyID;
typedef uint16_t DependencyCount;
typedef uint16_t LocationID;
typedef uint16_t LocationCount;
typedef uint16_t BalanceCostID;
typedef uint16_t BalanceCostCount;

#define CHECK(x) assert(x)

#endif

