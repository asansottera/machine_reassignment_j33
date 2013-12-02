#include <linear_solver_sub_problem.h>
#include <glpk.h>

using namespace std;
using namespace R12;

/*
* Returns the index of x_ij variable in the problem matrix. Process and machine values are assumed to start from 1
*/
uint32_t LinearSolverSubProblem::getMatrixIndexForVariable(uint32_t process_index, MachineID machine){
#ifdef LS_DEBUG
	CHECK(process_index < n_free_variables);
	CHECK(process_index >= 0 );	
	CHECK(machine <= n_machines);
	CHECK(machine > 0 );
#endif
	return process_index * n_machines + machine;
	}

/*
* Returns a vector containing processes of the given service.
* Services starts with index 0.
*/
void LinearSolverSubProblem::getProcessesOfService(ServiceID service, std::vector<ProcessID> & processes){
#ifdef LS_DEBUG
	CHECK(service >= 0);
	CHECK(service < n_services);
#endif
	for(ProcessID process = 0; process < n_processes; process++){
		if(instance().processes()[process].service() == service)
			processes.push_back(process);
	}
	CHECK(processes.size() > 0);
}


/*
* Returns a vector containing machines of the given location
* Locations starts with index 0
*/	
void LinearSolverSubProblem::getMachinesOfLocations(LocationID location, vector<MachineID>& machines){
#ifdef LS_DEBUG
	CHECK(location > 0);
	CHECK(location <= n_locations);
#endif

	for(MachineID machine = 0; machine < n_machines; machine++){
		if(instance().machines()[machine].location() == location)
			machines.push_back(machine);
	}
#ifdef LS_DEBUG
	//CHECK(machines.size() > 0);
#endif
}


/*
* Adds bin-packing constraints
*/
void LinearSolverSubProblem::addPlacementConstraints(){
	// adding variables x_ij
	glp_add_cols(lp, n_free_variables * n_machines);
	last_used_row = glp_add_rows(lp, n_free_variables);
	
	for(uint32_t free_process_index = 0; free_process_index <  n_free_variables; free_process_index++){
		
		int* col_indexes = new int[1 + n_machines];
		double* col_vals = new double[1 + n_machines];
		
		for(MachineID machine = 1; machine <= n_machines; machine++){
			int var_index = getMatrixIndexForVariable(free_process_index, machine);
			
			// x_ij is a binary variable
			glp_set_col_kind(lp, var_index, GLP_BV);
			col_indexes[machine] = var_index;			
			col_vals[machine] = 1.0;
		}
		
		int row = free_process_index + 1;
		glp_set_mat_row(lp, row, n_machines, col_indexes, col_vals);
		glp_set_row_bnds(lp, row, GLP_FX, 1.0, 0.0);
		delete[] col_indexes;
		delete[] col_vals;
	}
}

bool LinearSolverSubProblem::isFreeProcess(ProcessID process){
	for(ProcessID p = 0; p < n_free_variables; p++){
		if(m_freeProcesses[p] == process)
			return true;
	}
	return false;
}

/*
 * Capacity constraint for each machine
 */
void LinearSolverSubProblem::addCapacityConstraints(){
		
	// add capacity constraints
	last_used_row = glp_add_rows(lp, n_resources * n_machines);
	for(ResourceID resource = 1; resource <= n_resources ; resource++){
		for(MachineID machine = 1; machine <= n_machines; machine++){
			int* col_indexes = new int[1 + n_free_variables];
			double* col_vals = new double[1 + n_free_variables];
			
			for(uint32_t free_process_index = 0; free_process_index < n_free_variables; free_process_index++){
				col_indexes[free_process_index + 1] = getMatrixIndexForVariable(free_process_index, machine);
				ProcessID processID = m_freeProcesses[free_process_index];
				col_vals[free_process_index + 1] = instance().processes()[processID].requirement(resource - 1);		
			}
			
			glp_set_mat_row(lp, last_used_row, n_free_variables, col_indexes, col_vals);
			glp_set_row_bnds(lp, last_used_row, GLP_UP, 0.0, reduced_capacities[machine - 1][resource - 1]);
			++last_used_row;
			delete[] col_indexes;
			delete[] col_vals;
		}
	}
}

void LinearSolverSubProblem::addConflictConstraints(){
	// add conflict constraint
	// i.e. processes of the same service anno be placed on the same machine
	last_used_row = glp_add_rows(lp, n_machines);

	for(MachineID machine = 1; machine <= n_machines; machine++){
		int* col_indexes = new int[1 + n_free_variables];
		double* col_vals = new double[1 + n_free_variables];

		for(ProcessID free_process_index = 0; free_process_index < n_free_variables; free_process_index++){
			col_indexes[free_process_index + 1] = getMatrixIndexForVariable(free_process_index, machine);
			col_vals[free_process_index + 1] = 1.0;
		}
		glp_set_mat_row(lp, last_used_row, n_free_variables, col_indexes, col_vals);
		glp_set_row_bnds(lp, last_used_row, GLP_UP, 0.0, 1.0);
		last_used_row++;
		delete[]  col_indexes;
		delete[]  col_vals;
		}
}


void LinearSolverSubProblem::addLoadCostObjectiveFunction(){
	//lc_rj
	last_used_col = glp_add_cols(lp, n_machines * n_resources);
	
	for(ResourceID resource = 1; resource <= n_resources; resource++){
		double resource_weight = instance().resources()[resource - 1].weightLoadCost();
		
		for(MachineID machine = 1; machine <= n_machines; machine++){
			uint32_t lc_index = last_used_col++;

			glp_set_col_kind(lp, lc_index, GLP_CV);
			glp_set_col_bnds(lp, lc_index, GLP_LO, 0.0, 0.0);
			
			uint32_t size = n_free_variables + 1;
			int* col_indexes = new int[1 + size];
			double* col_vals = new double[1 + size];
			
			// compute utilization for machine m and resource r (sum over all processes)
			for(ProcessID free_process_index = 0; free_process_index < n_free_variables; free_process_index++){
				col_indexes[free_process_index + 1] = getMatrixIndexForVariable(free_process_index, machine);
				col_vals[free_process_index + 1] = instance().processes()[free_process_index].requirement(resource - 1);
			}
			// -LC_rm
			col_indexes[size] = lc_index;
			col_vals[size] = -1.0;

			uint32_t row = glp_add_rows(lp, 1);
			glp_set_mat_row(lp, row, size, col_indexes, col_vals);
			Machine m = instance().machines()[machine - 1];
			double row_bound =  m.safetyCapacity(resource - 1) + m.capacity(resource - 1) - reduced_capacities[machine - 1][resource - 1];
			glp_set_row_bnds(lp, row, GLP_UP, 0.0, row_bound);
			glp_set_obj_coef(lp, lc_index, resource_weight);
			delete[] col_indexes;
			delete[] col_vals;
		}
	}
}

void LinearSolverSubProblem::addSpreadConstraints(){
	// add spread constraints
	// slack variable (i.e. processes of s in location l)
	last_used_col = glp_add_cols(lp, n_locations);
	last_used_row = glp_add_rows(lp, n_locations);
	
	for(LocationID location = 1; location <= n_locations; location++){
		vector<MachineID> machinesOfLocation;
		getMachinesOfLocations(location, machinesOfLocation);
		int rowSize = 1 + n_free_variables * machinesOfLocation.size(); /* + 1 */
		int* col_indexes = new int[1 + rowSize];
		double* col_vals = new double[1 + rowSize];
		int column_index = 1;
		for(ProcessID process = 0; process < n_free_variables; process++){
			for(MachineID machine = 1; machine <= machinesOfLocation.size(); machine++){
				col_indexes[column_index] = getMatrixIndexForVariable(process, machine);
				col_vals[column_index] = 1.0;
				column_index++;
			}
		}
		col_indexes[column_index] = last_used_col;
		col_vals[column_index] = -1.0;
		glp_set_mat_row(lp, last_used_row, rowSize, col_indexes, col_vals);
		glp_set_row_bnds(lp, last_used_row, GLP_FX, 0.0, 0.0);
		glp_set_col_bnds(lp, last_used_col, GLP_LO, 0.0, 0.0);
		glp_set_col_kind(lp, last_used_col, GLP_CV);
		last_used_row++;
		last_used_col++;
		delete[] col_indexes;
		delete[] col_vals;
	}	


	//add variables to transform the slack variable to a binary variable
	uint32_t services_locations = n_locations;
	last_used_col = glp_add_cols(lp, services_locations);
	int sl_start_index = last_used_col;

	for(LocationID location = 1; location <= n_locations; location++){
			glp_set_col_kind(lp, last_used_col, GLP_BV);
			int* ind1 = new int[1 + 2];
			double* vals1 = new double[1 + 2];
			ind1[1] = last_used_col - services_locations;
			vals1[1] = -n_free_variables;
			ind1[2] = last_used_col;
			vals1[2] = 1.0;
			uint32_t last_row = glp_add_rows(lp, 1);
			glp_set_mat_row(lp, last_row, 2, ind1, vals1);
			glp_set_row_bnds(lp, last_row, GLP_LO, 0.0, 0.0);
			
			int* ind2 = new int[1 + 2];
			double* vals2 = new double[1 + 2];
			ind2[1] = last_used_col - services_locations;
			vals2[1] = 1.0;
			ind2[2] = last_used_col;
			vals2[2] = -n_free_variables;
			last_row = glp_add_rows(lp, 1);
			glp_set_mat_row(lp, last_row, 2, ind2, vals2);
			glp_set_row_bnds(lp, last_row, GLP_LO, 0.0, 0.0);			
			last_used_col++;

			delete[] ind1;
			delete[] vals1;
			delete[] ind2;
			delete[] vals2;		
	}
	

	int* ind = new int[1 + n_locations];
	double* vals = new double[1 + n_locations];
		
	for(LocationID location = 1; location <= n_locations; location++){
		ind[location] = sl_start_index + (location - 1);
		vals[location] = 1.0;
	}
	int row = glp_add_rows(lp, 1);
	glp_set_mat_row(lp, row, n_locations, ind, vals);
	glp_set_row_bnds(lp, row, GLP_LO, instance().services()[lockedService()].spreadMin(), 0.0);
	delete[] ind;
	delete[] vals;

}


void LinearSolverSubProblem::addDependencyConstraints(){
	// get dependencies for the locked service
	auto deps = boost::out_edges(lockedService(),  instance().dependency());
	if(deps.first == deps.second)
		return;

	// for each service dependent on the previous service
	for (auto depItr = deps.first; depItr != deps.second; ++depItr) {
		// dep is the service that need to be present in any neighborhood used by service s
		ServiceID dep = boost::target(*depItr,  instance().dependency());
		addDependecyConstraintBetweenServices(lockedService(), dep);
	}
}

void LinearSolverSubProblem::addDependecyConstraintBetweenServices(ServiceID &service1, ServiceID &service2){
	// for each neighborhood
	CHECK(service1 >= 0);
	CHECK(service1 < n_services);
	CHECK(service2 >= 0);
	CHECK(service2 < n_services);
	      
	vector<ProcessID> processesOfService1 = freeProcesses(); 
	vector<ProcessID> processesOfService2;
	getProcessesOfService(service1, processesOfService1);
	getProcessesOfService(service2, processesOfService2);

//	for(int i=0;i<processesOfService1.size();i++){
	//	cout<< processesOfService1[i]<<endl;
	//}
	for(NeighborhoodID neighborhood = 1; neighborhood <= n_neighborhoods; neighborhood++){
		vector<MachineID> machinesInNeighbor = instance().machinesByNeighborhood(neighborhood - 1);
		
		uint32_t size = machinesInNeighbor.size() * n_free_variables;

		int* ind = new int[1 + size];
		double* vals = new double[1 + size];
		uint32_t index = 1;

		// for all processes of the given service
		for(ProcessID process = 0; process < n_free_variables; process++){
			for(MachineID machine = 1; machine <= machinesInNeighbor.size(); machine++){
				int p = processesOfService1[process];
				MachineID m = machinesInNeighbor[machine - 1] + 1;
				ind[index] = getMatrixIndexForVariable(process, m);
				vals[index] = 1.0;
				index++;
			}
		}
		
		bool isDependentServiceInNeighborhood = false;
		for(ProcessID process = 0; process < processesOfService2.size(); process++){
			ProcessID p = processesOfService2[process];
			MachineID m = initial()[p];

			for(MachineID machine = 1; machine <= machinesInNeighbor.size(); machine++){
				if(machine - 1 == m)
					isDependentServiceInNeighborhood = true;
			}	
		}
		
		int row = glp_add_rows(lp, 1);
		glp_set_mat_row(lp, row, size, ind, vals);
		double upper_bound = isDependentServiceInNeighborhood ? n_free_variables : 0;
		glp_set_row_bnds(lp, row, GLP_UP, 0.0, upper_bound);
		delete[] ind;
		delete[] vals;
	} // neighbors

}

void LinearSolverSubProblem::addTransientUsageConstraints(){
	
	for(ResourceID resource = 1; resource <= n_resources; resource++){
		if(!instance().resources()[resource - 1].transient())
			continue;

		// transient resource
		for(MachineID machine = 1; machine <= n_machines; machine++){			
			
			double capacityLimit = (double) instance().machines()[machine - 1].capacity(resource - 1);	
			int* ind = new int[1 + n_free_variables];
			double* vals = new double[1 + n_free_variables];
			uint32_t index = 1;
			for(ProcessID process = 0; process < n_free_variables; process++){
				ind[index] = getMatrixIndexForVariable(process, machine);
				uint32_t resourceRequirement = instance().processes()[process].requirement(resource - 1);			
				if(initial()[process] == (machine - 1)){
					capacityLimit -= resourceRequirement;
					vals[index] = 0.0;
				} else {
					vals[index] = resourceRequirement;
				}
				index++;
				}
			int row = glp_add_rows(lp, 1);
			glp_set_mat_row(lp, row, n_free_variables, ind, vals);
			glp_set_row_bnds(lp, row, GLP_UP, 0.0,  capacityLimit);
			delete[] ind;
			delete[] vals;
		}
	}
}

uint32_t LinearSolverSubProblem::addProcessMoveCost(){
	//adding variable indicating wheter the given process was moved
	uint32_t init_col = glp_add_cols(lp, n_processes);
	uint32_t last_col = init_col;

	for(ProcessID process = 1; process <= n_processes; process++){
		glp_set_col_kind(lp, last_col, GLP_BV);
		uint32_t initialMachine = initial()[process - 1] + 1;
		uint32_t x_ij_col = getMatrixIndexForVariable(process, initialMachine);
		int* ind = new int[1 + 2];
		double* vals = new double[1 + 2];
		ind[1] = x_ij_col;
		vals[1] = 1.0;
		ind[2] = last_col;
		vals[2] = 1.0;
		uint32_t row = glp_add_rows(lp, 1);
		glp_set_mat_row(lp, row, 2, ind, vals);
		glp_set_row_bnds(lp, row, GLP_LO, 1.0, 0.0);
		double weight = instance().weightProcessMoveCost() * instance().processes()[process - 1].movementCost();
		glp_set_obj_coef(lp, last_col, weight);
		last_col++;

		delete[] ind;
		delete[] vals;
	}

	return init_col;
}



void LinearSolverSubProblem::addServiceMoveCost(uint32_t startColForProcessMove){
	uint32_t last_col = glp_add_cols(lp, 1);
	glp_set_col_kind(lp, last_col, GLP_CV);
	glp_set_col_bnds(lp, last_col, GLP_LO, 0.0, 0.0);
	
	for(ServiceID service = 1; service <= n_services; service++){
		vector<ProcessID> processesOfService;
		getProcessesOfService(service - 1, processesOfService);
		int* ind = new int[1 + processesOfService.size() + 1];
		double* vals = new double[1 + processesOfService.size() + 1];
		uint32_t index = 1;
		// for each process belonging to service
		for(vector<ProcessID>::iterator it = processesOfService.begin(); it != processesOfService.end(); ++it){
			ind[index] = startColForProcessMove + (*it);
			vals[index] = 1.0;
			index++;
		}
		ind[index] = last_col;
		vals[index] = -1.0;
		uint32_t row = glp_add_rows(lp, 1);
		glp_set_mat_row(lp, row, processesOfService.size() + 1, ind, vals);
		glp_set_row_bnds(lp, row, GLP_UP, 0.0, 0.0);
		delete[] ind;
		delete[] vals;
	}
	glp_set_obj_coef(lp, last_col, instance().weightServiceMoveCost());
}

void LinearSolverSubProblem::addMachineMoveCost(){
	uint32_t last_col = glp_add_cols(lp, 1);
	glp_set_col_kind(lp, last_col, GLP_CV);
	glp_set_col_bnds(lp, last_col, GLP_LO, 0.0, 0.0);
	
	uint32_t size = n_free_variables * n_machines + 1;
	int* ind = new int[1 + size];
	double* vals = new double[1 + size];
	uint32_t index = 1;
	for(ProcessID process = 0; process < n_free_variables; process++){
		MachineID initialMachine = initial()[process] + 1;
		for(MachineID machine = 1; machine <= n_machines; machine++){
			ind[index] = getMatrixIndexForVariable(process, machine);
			vals[index] = instance().machineMoveCost(initialMachine - 1, machine - 1);
			index++;
		}
	}
	ind[index] = last_col;
	vals[index] = -1.0;
	
	uint32_t row = glp_add_rows(lp, 1);
	glp_set_mat_row(lp, row, size, ind, vals);
	glp_set_row_bnds(lp, row, GLP_UP, 0.0, 0.0);
	glp_set_obj_coef(lp, last_col, instance().weightMachineMoveCost());
	delete[] ind;
	delete[] vals;
}

void LinearSolverSubProblem::addBalanceCost(){
	
	if(n_balanceCosts == 0)
		return;
	
	uint32_t init_col = glp_add_cols(lp, n_machines * n_balanceCosts);
	uint32_t last_col = init_col;
	
	for(uint32_t balanceCost = 0; balanceCost < n_balanceCosts; balanceCost++){
		BalanceCost b = instance().balanceCosts()[balanceCost];
		
		for(MachineID machine = 1; machine <= n_machines; machine++){
			uint32_t size = n_processes + 1;
			int* ind = new int[1 + size];
			double* vals = new double[1 + size];
			uint32_t index = 1;
			for(ProcessID process = 1; process <= n_processes; process++){
				ind[index] = getMatrixIndexForVariable(process, machine);
				Process p = instance().processes()[process - 1];
				vals[index] = -p.requirement(b.resource2()) + b.target() * p.requirement(b.resource1());
				index++;
			}// end processes
			ind[index] = last_col;
			vals[index] = 1;
			
			int row = glp_add_rows(lp, 1);
			glp_set_mat_row(lp, row, size, ind, vals);
			Machine m = instance().machines()[machine - 1];
			
			double bound = b.target() * m.capacity(b.resource1()) - m.capacity(b.resource2());
			glp_set_row_bnds(lp, row, GLP_LO, bound , 0.0);
			glp_set_col_kind(lp, last_col, GLP_CV);
			glp_set_col_bnds(lp, last_col, GLP_LO, 0.0, 0.0);
			
#ifdef LS_DEBUG
			stringstream var_name;
			var_name << "bc_" << balanceCost+1 << machine;
			glp_set_col_name(lp, last_col, var_name.str().c_str());
#endif
			glp_set_obj_coef(lp, last_col, b.weight());
			last_col++;
			delete[] ind;
			delete[] vals;
		} // end for machines
	}
}


/*
 * Set major properties for GLPK
 */
void LinearSolverSubProblem::initGLPK(){
	lp = glp_create_prob();
	glp_set_prob_name(lp, "gchallenge");	
	glp_init_iocp(&param);
	setSolverParameters();

	// minimization problem
	glp_set_obj_dir(lp, GLP_MIN);
}

void LinearSolverSubProblem::setSolverParameters(){
	param.presolve = GLP_ON;
	/*
	param.fp_heur = GLP_ON;
	param.gmi_cuts = GLP_ON;
	param.mir_cuts = GLP_ON;
	param.cov_cuts = GLP_ON;
	param.clq_cuts = GLP_ON;
	param.tm_lim = 1000 * 60;
	*/
}

void LinearSolverSubProblem::computeReducedCapacities(){
	//scale capacity
	
	for(MachineID m = 0; m < n_machines; m++){
		vector<uint32_t> v;
		for(ResourceID r = 0; r < n_resources; r++){
			uint32_t reduced_capacity = instance().machines()[m].capacity(r);
			for(ProcessID p = 0; p < n_processes; p++){
				// this can be improved by iterating on the list of fixed processes
				if(!isFreeProcess(p)){
					reduced_capacity -= instance().processes()[p].requirement(r);
				}
			}
			v.push_back(reduced_capacity);
		}
		reduced_capacities.push_back(v);
	}
}

void LinearSolverSubProblem::solve(){	
	initGLPK();
	
	n_processes = instance().processes().size();
	n_machines = instance().machines().size();
	n_resources = instance().resources().size();
	n_services = instance().services().size();
	n_neighborhoods = instance().neighborhoodCount();
	n_locations = instance().locationCount();
	n_balanceCosts = instance().balanceCosts().size();	

	computeReducedCapacities();
	addPlacementConstraints();
	addCapacityConstraints();
	addConflictConstraints();
	addLoadCostObjectiveFunction();
	addSpreadConstraints();
	addDependencyConstraints();
	addTransientUsageConstraints();
	addMachineMoveCost();		
	//uint32_t startColForProcessMove = addProcessMoveCost();
	//addServiceMoveCost(startColForProcessMove);
	//addBalanceCost();

	//solve the problem
	glp_intopt(lp, &param);
	
#ifdef LS_DEBUG		
	cout<< "Objective: " << glp_mip_obj_val(lp)<< endl;
#endif

	vector<MachineID> machineAssignment = initial();
	for(ProcessID free_process_index = 0; free_process_index < n_free_variables; free_process_index++){
		for(MachineID machine = 0; machine < n_machines; machine++){
			double x_ij = glp_mip_col_val(lp, getMatrixIndexForVariable(free_process_index, machine + 1));
			if(x_ij == 1.0){
				machineAssignment[free_process_index] = machine;
			}
		}
	}

	SolutionInfo sol(instance(), initial(), machineAssignment);
	
#ifdef LS_DEBUG
	Verifier verifier;
	Verifier::Result result = verifier.verify(instance(), initial(), machineAssignment);
	if (!result.feasible()) {
		throw runtime_error("Linear solver: wrong feasibility");
	} else {
		cout<< "Solution is feasible with obj: "<< sol.objective()<< endl;
	}
#endif

//	best_objective = sol.objective();
//	best_solution = sol.solution();


	//deallocate glpk problem
	glp_delete_prob(lp);
}
