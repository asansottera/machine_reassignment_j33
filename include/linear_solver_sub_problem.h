#ifndef R12_LINEAR_PROBLEM_SUB_PROBLEM_H
#define R12_LINEAR_PROBLEM_SUB_PROBLEM_H

#include <glpk.h>
#include "common.h"
#include "heuristic.h"
#include "verifier.h"


#define LS_DEBUG 1

namespace R12
{

class LinearSolverSubProblem {

	public:
		LinearSolverSubProblem(const Problem & instance, const std::vector<MachineID> &initial, ServiceID & locked_service) {
			m_instancePtr = &instance;
			m_initial = initial;
			m_freeProcesses = instance.processesByService(locked_service);
			m_locked_servicePtr = &locked_service;
			n_free_variables = m_freeProcesses.size();
			CHECK(n_free_variables > 1);
		};
		void solve();

		
	private:
		const Problem* m_instancePtr;
		std::vector<MachineID> m_initial;
		std::vector<ProcessID> m_freeProcesses;
		ServiceID* m_locked_servicePtr;
		
		const Problem & instance(){
			return (*m_instancePtr);
		}

		const std::vector<MachineID> initial(){
			return m_initial;
		}

		const std::vector<ProcessID> freeProcesses(){
			return m_freeProcesses;
		}
		
		ServiceID& lockedService(){
			return (*m_locked_servicePtr);
		}
			
		uint32_t n_free_variables;
		uint32_t n_processes;
		uint32_t n_machines;
		uint32_t n_resources;
		uint32_t n_services;
		uint32_t n_neighborhoods;
		uint32_t n_locations;
		uint32_t n_balanceCosts;

		std::vector<std::vector<uint32_t>> reduced_capacities;

		uint32_t getMatrixIndexForVariable(uint32_t process, MachineID machine);
		void getProcessesOfService(ServiceID service, std::vector<ProcessID> & processes);
		void getMachinesOfLocations(LocationID location, std::vector<MachineID> & machines);
		bool isFreeProcess(ProcessID process);
		void computeReducedCapacities();

		
		// GLPK data
		glp_prob *lp;
		// solver parameters
		glp_iocp param;

		uint32_t last_used_row;
		uint32_t last_used_col;
		//uint32_t load_cost_col;
		//uint32_t balance_cost_col;

		void setSolverParameters();
		void initGLPK();
		
		// model creation
		void addPlacementConstraints();
		void addCapacityConstraints();
		void addConflictConstraints();
		void addLoadCostObjectiveFunction();
		void addSpreadConstraints();
		void addDependencyConstraints();
		void addDependecyConstraintBetweenServices(ServiceID & service1, ServiceID & service2);
		void addTransientUsageConstraints();
		uint32_t addProcessMoveCost();
		void addServiceMoveCost(uint32_t startColForProcessMove);
		void addMachineMoveCost();
		void addBalanceCost();
	};
}

#endif