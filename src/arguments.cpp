// project headers
#include "arguments.h"
// standard library headers
#include <string>
#include <iostream>
// boost headers
#include <boost/program_options.hpp>

#ifdef __INTEL_COMPILER
#pragma warning(disable:2259)
#endif

using namespace R12;
using namespace std;
using namespace boost;

#define DEFAULT_HEURISTIC "vns3#ls=optimized:ls@maxSamples=10000,simulated_annealing"

pair<string,string> parseOptionName(const string & s) {

	if (s.compare("-name") == 0) {
		return make_pair("name", "");
	}

	return make_pair(string(), string());
}

bool Arguments::parse(int argc, char ** argv) {
	
	program_options::options_description desc("Program options");
	desc.add_options()
		("time-limit,t", program_options::value<uint32_t>(),
			"Time limit (in seconds) after which the program terminates. The time measured is real (wall-clock) time.")
		("problem-instance,p", program_options::value<string>(),
			"The path of the problem instance file.")
		("input-solution,i", program_options::value<string>(),
			"The path to read the initial solution from.")
		("output-solution,o", program_options::value<string>(),
			"The path to write the new solution to.")
		("seed,s", program_options::value<unsigned int>(),
			"The seed to use for random numer generation.")
		("name,n",
			"The program outputs the team identifier. The syntax \"-name\" is also recognized.")
		("heuristic,h", program_options::value<string>(),
			"The name of the heuristic algorithm.")
		("analyze,a",
			"Displays information about the problem and its solution, then exits.");
	program_options::positional_options_description pdesc;
	program_options::variables_map vm;
	try {
		program_options::store(
			program_options::command_line_parser(argc, argv)
				.options(desc)
				.positional(pdesc)
				// "-name" needs a custom parser because the single dash before a long option is not standard
				.extra_parser(parseOptionName)
				.run(),
			vm);
	} catch (const program_options::error & error) {
		cerr << "Invalid program options: " << error.what() << endl;
		cerr << desc << endl;
		return false;
	}
	program_options::notify(vm);

	if (vm.count("name") > 0) {
		printName = true;
	} else {
		printName = false;
	}

	// if "-name" is the only option, return the team identifier and exit
	if (argc == 2 && printName) {
		onlyPrintName = true;
		return true;
	} else {
		onlyPrintName = false;
	}

	if (vm.count("analyze") > 0) {
		analyze = true;
	} else {
		analyze = false;
	}

	if ((vm.count("time-limit") > 0 || vm.count("heuristic") > 0 || vm.count("seed") > 0) && vm.count("analyze") > 0) {
		cerr << "Invalid program options: analyze mode does not support options \"time-limit\", \"heuristic\" and \"seed\"." << endl;
		return false;
	}

	if (!analyze) {
		if (vm.count("time-limit") > 0) {
			timeLimit = vm["time-limit"].as<uint32_t>();
		} else {
			cerr << "Invalid program options: time limit required." << endl;
			cerr << desc << endl;
			return false;
		}

		if (vm.count("seed") > 0) {
			seed = vm["seed"].as<unsigned int>();
		} else {
			seed = 0;
		}

		if (vm.count("heuristic") > 0) {
			heuristicName = vm["heuristic"].as<string>();
		} else {
			heuristicName = DEFAULT_HEURISTIC;
		}
	}

	if (vm.count("problem-instance") > 0) {
		problemInstance = vm["problem-instance"].as<string>();
	}  else {
		cerr << "Invalid program options: problem instance required" << endl;
		cerr << desc << endl;
		return false;
	}

	if (vm.count("input-solution") > 0) {
		inputSolution = vm["input-solution"].as<string>();
	} else {
		cerr << "Invalid program options: input solution required" << endl;
		cerr << desc << endl;
		return false;
	}

	if (vm.count("output-solution") > 0) {
		outputSolution = vm["output-solution"].as<string>();
	} else {
		cerr << "Invalid program options: output solution required" << endl;
		cerr << desc << endl;
		return false;
	}

	return true;
}

