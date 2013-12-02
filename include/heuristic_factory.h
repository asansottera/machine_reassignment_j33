#ifndef R12_HEURISTIC_FACTORY_H
#define R12_HEURISTIC_FACTORY_H

#include "heuristic.h"
#include <string>
#include <stdexcept>

namespace R12 {

Heuristic * makeHeuristic(const std::string & name);

}

#endif
