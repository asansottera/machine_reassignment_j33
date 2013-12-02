#include "analyzer.h"
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

using namespace R12;
using namespace boost::accumulators;

void Analyzer::analyze(const SolutionInfo & solution, std::ostream & out) {
	out << "Number of resources: " << solution.instance().resources().size() << std::endl;
	out << "Number of transient resources: " << solution.instance().transientResources().size() << std::endl;
	out << "Number of balance costs: " << solution.instance().balanceCosts().size() << std::endl;
	out << "Number of processes: " << solution.instance().processes().size() << std::endl;
	out << "Number of services: " << solution.instance().services().size() << std::endl;
	out << "Number of machines: " << solution.instance().machines().size() << std::endl;
	out << "Number of neighborhoods: " << solution.instance().neighborhoodCount() << std::endl;
	out << "Number of locations: " << solution.instance().locationCount() << std::endl;
	// processes per service
	accumulator_set<ProcessCount, stats< tag::min, tag::mean, tag::max > > pPerServiceAcc;
	for (ServiceID s = 0; s < solution.instance().services().size(); ++s) {
		ProcessCount pc = static_cast<ProcessCount>(solution.instance().processesByService(s).size());
		pPerServiceAcc(pc);
	}
	out << "Processes per service: ";
	out << "min = " << min(pPerServiceAcc) << ", ";
	out << "max = " << max(pPerServiceAcc) << ", ";
	out << "mean = " << mean(pPerServiceAcc) << std::endl;
	// dependencies per service
	accumulator_set<ServiceCount, stats< tag::min, tag::mean, tag::max > > depPerServiceAcc;
	for (ServiceID s = 0; s < solution.instance().services().size(); ++s) {
		ServiceCount sc = boost::out_degree(s, solution.instance().dependency());
		depPerServiceAcc(sc);
	}
	out << "Dependencies per service: ";
	out << "min = " << min(depPerServiceAcc) << ", ";
	out << "max = " << max(depPerServiceAcc) << ", ";
	out << "mean = " << mean(depPerServiceAcc) << std::endl;
	// reverse dependencies per service
	accumulator_set<ServiceCount, stats< tag::min, tag::mean, tag::max > > rdepPerServiceAcc;
	for (ServiceID s = 0; s < solution.instance().services().size(); ++s) {
		ServiceCount sc = boost::in_degree(s, solution.instance().dependency());
		rdepPerServiceAcc(sc);
	}
	out << "Reverse dependencies per service: ";
	out << "min = " << min(rdepPerServiceAcc) << ", ";
	out << "max = " << max(rdepPerServiceAcc) << ", ";
	out << "mean = " << mean(rdepPerServiceAcc) << std::endl;
	// machines per location
	accumulator_set<MachineCount, stats< tag::min, tag::mean, tag::max > > mPerLocationAcc;
	for (LocationID l = 0; l < solution.instance().locationCount(); ++l) {
		MachineCount count = 0;
		for (MachineID m = 0; m < solution.instance().machines().size(); ++m) {
			const Machine & machine = solution.instance().machines()[m];
			if (machine.location() == l) {
				++count;
			}
		}
		mPerLocationAcc(count);
	}
	out << "Machines per location: ";
	out << "min = " << min(mPerLocationAcc) << ", ";
	out << "max = " << max(mPerLocationAcc) << ", ";
	out << "mean = " << mean(mPerLocationAcc) << std::endl;
	// machines per neighborhood
	accumulator_set<MachineCount, stats< tag::min, tag::mean, tag::max > > mPerNeighborhoodAcc;
	for (NeighborhoodID n = 0; n < solution.instance().neighborhoodCount(); ++n) {
		MachineCount count = 0;
		for (MachineID m = 0; m < solution.instance().machines().size(); ++m) {
			const Machine & machine = solution.instance().machines()[m];
			if (machine.neighborhood() == n) {
				++count;
			}
		}
		mPerNeighborhoodAcc(count);
	}
	out << "Machines per neighborhood: ";
	out << "min = " << min(mPerNeighborhoodAcc) << ", ";
	out << "max = " << max(mPerNeighborhoodAcc) << ", ";
	out << "mean = " << mean(mPerNeighborhoodAcc) << std::endl;
	// resource requirement
	for (ResourceID r = 0; r < solution.instance().resources().size(); ++r) {
		accumulator_set<uint32_t, stats< tag::min, tag::mean, tag::max, tag::variance > > reqPerProcessAcc;
		for (ProcessID p = 0; p < solution.instance().processes().size(); ++p) {
			const Process & process = solution.instance().processes()[p];
			reqPerProcessAcc(process.requirement(r));
		}
		out << "Resource " << r << " requirement per process: ";
		out << "min = " << min(reqPerProcessAcc) << ", ";
		out << "max = " << max(reqPerProcessAcc) << ", ";
		out << "mean = " << mean(reqPerProcessAcc) << ", ";
		out << "variance = " << variance(reqPerProcessAcc) << std::endl;
	}
	// resource capacity
	for (ResourceID r = 0; r < solution.instance().resources().size(); ++r) {
		accumulator_set<uint32_t, stats< tag::min, tag::mean, tag::max, tag::variance > > capPerMachineAcc;
		for (MachineID m = 0; m < solution.instance().machines().size(); ++m) {
			const Machine & machine = solution.instance().machines()[m];
			capPerMachineAcc(machine.capacity(r));
		}
		out << "Resource " << r << " capacity per machine: ";
		out << "min = " << min(capPerMachineAcc) << ", ";
		out << "max = " << max(capPerMachineAcc) << ", ";
		out << "mean = " << mean(capPerMachineAcc) << ", ";
		out << "variance = " << variance(capPerMachineAcc) << std::endl;
	}
	// safety capacity fraction
	for (ResourceID r = 0; r < solution.instance().resources().size(); ++r) {
		accumulator_set<double, stats< tag::min, tag::mean, tag::max, tag::variance > > safetyPerMachineAcc;
		for (MachineID m = 0; m < solution.instance().machines().size(); ++m) {
			const Machine & machine = solution.instance().machines()[m];
			double safety = (static_cast<double>(machine.capacity(r)) - static_cast<double>(machine.safetyCapacity(r))) / static_cast<double>(machine.capacity(r));
			safetyPerMachineAcc(safety);
		}
		out << "Resource " << r << " safety fraction per machine: ";
		out << "min = " << min(safetyPerMachineAcc) << ", ";
		out << "max = " << max(safetyPerMachineAcc) << ", ";
		out << "mean = " << mean(safetyPerMachineAcc) << ", ";
		out << "variance = " << variance(safetyPerMachineAcc) << std::endl;
	}
	// cost structure
	out << "Total cost: " << solution.objective() << std::endl;
	for (ResourceID r = 0; r < solution.instance().resources().size(); ++r) {
		const Resource & resource = solution.instance().resources()[r];
		out << "Resource " << r << " load cost: " << resource.weightLoadCost() << " * " << solution.loadCost(r);
		out << " = " << resource.weightLoadCost() * solution.loadCost(r) << std::endl;
	}
	for (BalanceCostID b = 0; b < solution.instance().balanceCosts().size(); ++b) {
		const BalanceCost & balance = solution.instance().balanceCosts()[b];
		out << "Balance cost between resources " << balance.resource1() << " and " << balance.resource2() << ": ";
		out << balance.weight() << " * " << solution.balanceCost(b);
		out << " = " << balance.weight() * solution.balanceCost(b) << std::endl;
	}
	out << "Process move cost: " << solution.instance().weightProcessMoveCost() << " * " << solution.processMoveCost();
	out << " = " << solution.instance().weightProcessMoveCost() * solution.processMoveCost() << std::endl;
	out << "Machine move cost: " << solution.instance().weightMachineMoveCost() << " * " << solution.machineMoveCost();
	out << " = " << solution.instance().weightMachineMoveCost() * solution.machineMoveCost() << std::endl;
	out << "Service move cost: " << solution.instance().weightServiceMoveCost() << " * " << solution.serviceMoveCost();
	out << " = " << solution.instance().weightServiceMoveCost() * solution.serviceMoveCost() << std::endl;
}
