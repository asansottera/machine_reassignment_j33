#ifndef R12_ARGUMENTS_H
#define R12_ARGUMENTS_H

#include <string>
#include <cstdint>

#ifdef WIN32
#include <iostream>
#endif

namespace R12 {

struct Arguments {
	uint32_t timeLimit;
	std::string problemInstance;
	std::string inputSolution;
	std::string outputSolution;
	bool printName;
	bool onlyPrintName;
	unsigned int seed;
	std::string heuristicName;
	bool analyze;
	bool parse(int argc, char ** argv);
};

}

#endif
