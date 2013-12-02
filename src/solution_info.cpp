#include "solution_info.h"
#include "common.h"

using namespace R12;

SolutionInfo::SolutionInfo(const Problem & instance, const std::vector<MachineID> & initial)
	: m_instancePtr(&instance), m_initialPtr(&initial), m_solution(initial) {
	// precondition
	CHECK(instance.processes().size() == initial.size());
	initializeContainers();
	initialize();
	m_processMoveCost = 0;
	m_serviceMoveCost = 0;
	m_machineMoveCost = 0;
}

SolutionInfo::SolutionInfo(const Problem & instance, const std::vector<MachineID> & initial, const std::vector<MachineID> & solution)
	: m_instancePtr(&instance), m_initialPtr(&initial), m_solution(solution) {
	// precondition
	CHECK(instance.processes().size() == initial.size());
	CHECK(instance.processes().size() == solution.size());
	initializeContainers();
	initialize();
	initializeSolutionDelta();
}

void SolutionInfo::initializeContainers() {
	m_usage.resize(instance().machines().size());
	m_transient.resize(instance().machines().size());
	for (MachineID m = 0; m < instance().machines().size(); ++m) {
		m_usage[m].resize(instance().resources().size());
		m_transient[m].resize(instance().resources().size());
	}
	m_spread.resize(instance().services().size());
	m_boolMachinePresence.resize(instance().services().size());
	m_machinePresence.resize(instance().services().size());
	m_locationPresence.resize(instance().services().size());
	m_neighborhoodPresence.resize(instance().services().size());
	for (ServiceID s = 0; s < instance().services().size(); ++s) {
		m_boolMachinePresence[s].resize(instance().machines().size());
		m_machinePresence[s].resize(instance().machines().size());
		m_neighborhoodPresence[s].resize(instance().neighborhoodCount());
		m_locationPresence[s].resize(instance().locationCount());
	}
	m_movedProcesses.resize(instance().services().size());
	m_loadCosts.resize(instance().resources().size());
	m_balanceCosts.resize(instance().balanceCosts().size());
}

void SolutionInfo::initialize() {
	for (ProcessID p = 0; p < instance().processes().size(); ++p) {
		MachineID m = solution()[p];
		const Process & process = instance().processes()[p];
		const Machine & machine = instance().machines()[m];
		for (ResourceID r = 0; r < instance().resources().size(); ++r) {
			setUsage(m, r, usage(m, r) + process.requirement(r));
		}
		setBoolMachinePresence(process.service(), m, true);
		setMachinePresence(process.service(), m,
			machinePresence(process.service(), m) + 1);
		setNeighborhoodPresence(process.service(), machine.neighborhood(),
			neighborhoodPresence(process.service(), machine.neighborhood()) + 1);
		setLocationPresence(process.service(), machine.location(),
			locationPresence(process.service(), machine.location()) + 1);
	}
	for (MachineID m = 0; m < instance().machines().size(); ++m) {
		const Machine & machine = instance().machines()[m];
		for (ResourceID r = 0; r < instance().resources().size(); ++r) {
			setLoadCost(r,
				loadCost(r) + _computeLoadCost(usage(m, r), machine.safetyCapacity(r)));
		}
		for (BalanceCostID b = 0; b < instance().balanceCosts().size(); ++b) {
			const BalanceCost & balance = instance().balanceCosts()[b];
			setBalanceCost(b,
				balanceCost(b) + _computeBalanceCost(balance, machine, usage(m, balance.resource1()), usage(m, balance.resource2())));
		}
	}
}

void SolutionInfo::initializeSolutionDelta() {
	m_processMoveCost = 0;
	m_machineMoveCost = 0;
	m_serviceMoveCost = 0;
	for (ProcessID p = 0; p < instance().processes().size(); ++p) {
		MachineID mInitial = initial()[p];
		MachineID m = solution()[p];
		const Process & process = instance().processes()[p];
		if (mInitial != m) {
			for (ResourceID r = 0; r < instance().resources().size(); ++r) {
				const Resource & resource = instance().resources()[r];
				if (resource.transient()) {
					setTransient(mInitial, r, transient(mInitial, r) + process.requirement(r));
				}
			}
			setProcessMoveCost(processMoveCost() + process.movementCost());
			setMachineMoveCost(machineMoveCost() + instance().machineMoveCost(mInitial, m));
			setMovedProcesses(process.service(), movedProcesses(process.service()) + 1);
		}
	}
	computeServiceMoveCost();
}

bool SolutionInfo::check() const {
	return *this == SolutionInfo(instance(), initial(), solution());
}

bool SolutionInfo::operator==(const SolutionInfo & other) const {
	if (&instance() != &other.instance()) return false;
	if (&initial() != &other.initial()) return false;
	if (m_usage != other.m_usage) return false;
	if (m_transient != other.m_transient) return false;
	if (m_spread != other.m_spread) return false;
	if (m_boolMachinePresence != other.m_boolMachinePresence) return false;
	if (m_machinePresence != other.m_machinePresence) return false;
	if (m_locationPresence != other.m_locationPresence) return false;
	if (m_neighborhoodPresence != other.m_neighborhoodPresence) return false;
	if (m_movedProcesses != other.m_movedProcesses) return false;
	if (m_loadCosts != other.m_loadCosts) return false;
	if (m_balanceCosts != other.m_balanceCosts) return false;
	if (m_processMoveCost != other.m_processMoveCost) return false;
	if (m_serviceMoveCost != other.m_serviceMoveCost) return false;
	if (m_machineMoveCost != other.m_machineMoveCost) return false;
	return true;
}
