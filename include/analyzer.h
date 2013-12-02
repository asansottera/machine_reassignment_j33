#ifndef R12_ANALYZER_H
#define R12_ANALYZER_H

#include  "solution_info.h"
#include <iostream>

namespace R12 {

class Analyzer {
public:
	static void analyze(const SolutionInfo & solution, std::ostream & out);
};

}

#endif
