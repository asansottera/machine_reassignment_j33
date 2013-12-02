// project headers
#include "common.h"
#include "arguments.h"
#include "problem.h"
#include "verifier.h"
#include "heuristic.h"
#include "heuristic_factory.h"
#include "solution_pool.h"
#include "analyzer.h"
#include "atomic_flag.h"
// standard library headers
#include <cassert>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <ctime>
#include <iostream>
// boost headers
#include <boost/algorithm/string.hpp>


using namespace std;

/*! Reads a vector of space-separeted integer values from a text file. */
template<class T>
void readVector(const std::string & name, vector<T> & values) {

	ifstream inputFile(name);
	if (inputFile.fail()) {
		throw runtime_error("Unable to read input file");
	}
	
	T value = 0;
	while(inputFile >> value) {
		values.push_back(value);
	}
	CHECK(inputFile.eof());

	inputFile.close();
}

void writeVector(const std::string & name, const std::vector<MachineID> & solution) {

	ofstream outputFile(name);
	if (outputFile.fail()) {
		throw runtime_error("Unable to open output file");
	}

	for (auto itr = solution.begin(); itr != solution.end(); ++itr) {
		outputFile << *itr << " ";
	}

	outputFile.close();
}

#define TEAM_ID "J33"
#define TIME_SAFETY_GAP 2

// #define TRACE_MAINHH

#define R12_ERROR_READ_INPUT -1
#define R12_ERROR_HEURISTIC_PARSE -2
#define R12_ERROR_HEURISTIC_INIT -3
#define R12_ERROR_HEURISTIC_RUN -4

int main(int argc, char ** argv) {

	static const std::size_t maxHighQuality = 100;
	static const std::size_t maxHighDiversity = 20;
	static const ProcessCount hqMinBestDelta = 2;
	static const double hdMaxBestObjRatio = 1.1;

	// initialize deadline with starting time
	timespec deadline;	
	#ifdef _WIN32
	deadline.tv_sec = time(NULL);
	deadline.tv_nsec = 0;
	#else
	clock_gettime(CLOCK_REALTIME, &deadline);
	#endif

	R12::Arguments args;
	bool successful = args.parse(argc, argv);
	if (!successful) {
		return -1;
	}

	if (args.printName) {
		std::cout << TEAM_ID << endl;
	}

	if (args.onlyPrintName) {
		return 0;
	}

	// increase deadline according to time limit
	deadline.tv_sec += args.timeLimit - TIME_SAFETY_GAP;

	vector<uint32_t> instanceRaw;
	vector<MachineID> initial;

	#ifdef TRACE_MAINHH
	std::cout << "Reading and parsing model " << args.problemInstance;
	std::cout << " and initial solution " << args.inputSolution << std::endl;
	#endif
	try {
		readVector<uint32_t>(args.problemInstance, instanceRaw);
		readVector<MachineID>(args.inputSolution, initial);
	} catch (std::exception & ex) {
		std::cerr << "Error while reading input files: " << ex.what() << std::endl;
		return R12_ERROR_READ_INPUT;
	}
	R12::Problem instance = R12::Problem::parse(instanceRaw);
	
	#ifdef TRACE_MAINHH
	std::cout << "Objective lower bound: " << instance.lowerBoundObjective() << std::endl;
	#endif

	if (args.analyze) {
		vector<MachineID> solution;
		readVector<MachineID>(args.outputSolution, solution);
		R12::Analyzer::analyze(R12::SolutionInfo(instance, initial, solution), std::cout);
		return 0;
	}

	R12::SolutionPool pool(maxHighQuality, maxHighDiversity, hqMinBestDelta, hdMaxBestObjRatio);

	{
		// compute the initial objective value and store the initial solution in the pool
		uint64_t initialObjective = 0;
		R12::SolutionInfo initialInfo(instance, initial);
		initialObjective = initialInfo.objective();
		pool.push(initialObjective, initial);
	}

	std::vector<std::string> heuristicNames;
	std::vector<std::unique_ptr<R12::Heuristic>> heuristics;

	// get heuristic names
	boost::split(heuristicNames, args.heuristicName, boost::is_any_of(","));

	// shared atomic flag for termination
	R12::AtomicFlag flag;

	// create and initialize heuristics
	uint32_t seed = args.seed;
	for (auto itr = heuristicNames.begin(); itr != heuristicNames.end(); ++itr) {
		// get heuristic name and configuration string
		const std::string & hc = *itr;
		std::vector<std::string> nameAndConfig;
		boost::split(nameAndConfig, hc, boost::is_any_of("#"));
		if (nameAndConfig.size() > 2) {
			std::cerr << "Invalid heuristic configuration: " << hc << std::endl;
			return R12_ERROR_HEURISTIC_PARSE;
		}
		const std::string & name = nameAndConfig[0];
		const std::string & config = nameAndConfig.size() == 2 ? nameAndConfig[1] : "";
		// initialize heuristic
		try {
			std::unique_ptr<R12::Heuristic> h(R12::makeHeuristic(name));
			h->init(instance, initial, seed, flag, pool, config);
			heuristics.push_back(std::move(h));
		} catch (std::exception & ex) {
			std::cerr << "Error during initialization of heuristic ";
			std::cerr << (itr-heuristicNames.begin()) << " of type '";
			std::cerr << name << "': " << ex.what() << std::endl;
			return R12_ERROR_HEURISTIC_INIT;
		}
		// change seed for the next heuristic
		seed += 100;
	}

	// start heuristics
	for (auto hItr = heuristics.begin(); hItr != heuristics.end(); ++hItr) {
		(*hItr)->start(deadline);
	}

	// wait for completion of all heuristics or deadline
	std::vector<bool> completed(heuristics.size());
	uint32_t i = 0;
	for (auto hItr = heuristics.begin(); hItr != heuristics.end(); ++hItr) {
		completed[i] = (*hItr)->wait();
		++i;
	}

	// check if heuristics have completed heuristics
	#ifdef TRACE_MAINHH
	i = 0;
	for (auto hItr = heuristics.begin(); hItr != heuristics.end(); ++hItr) {
		if (!completed[i]) {
			std::cout << "Heuristic " << i << " not completed within time limit" << std::endl;
		} else {
			std::cout << "Heuristic " << i << " completed within time limit" << std::endl;
		}
		++i;
	}
	#endif

	// force termination by writing to the shared atomic flag
	#ifdef TRACE_MAINHH
	std::cout << "Forcing termination of uncompleted heuristics" << std::endl;
	#endif
	pool.shutdown();
	flag.write();

	// wait for termination
	i = 0;
	for (auto hItr = heuristics.begin(); hItr != heuristics.end(); ++hItr) {
		if (!completed[i]) {
			(*hItr)->join();
		}
		++i;
	}

	i = 0;
	int errorCount = 0;
	for (auto hItr = heuristics.begin(); hItr != heuristics.end(); ++hItr) {
		if ((*hItr)->error()) {
			std::cerr << "Error during the execution of heuristic ";
			std::cerr << i << ": " << (*hItr)->errorMessage() << std::endl;
			++errorCount;
		}
		++i;
	}
	if (errorCount > 0) {
		return R12_ERROR_HEURISTIC_RUN;
	}

	// get the best solution in the pool
	R12::SolutionPool::Entry best;
	if (!pool.best(best)) {
		throw std::runtime_error("No high quality solution in the pool!");
	}

	// verify solution and print objective
	#ifdef TRACE_MAINHH
	R12::Verifier::Result hVerifyResult = verifier.verify(instance, initial, *(best.ptr()));
	cout << (hVerifyResult.feasible() ? "Heuristic solution is feasible" : "Heuristic solution is not feasible") << endl;
	if (hVerifyResult.feasible()) {
		cout << "Heuristic objective value: " << hVerifyResult.objective() << endl;
	}
	#endif

	// write the best solution to the output file
	writeVector(args.outputSolution, *(best.ptr()));

	return 0;
}
