#include <linear_solver.h>

#include <iostream>
#include <sstream> 
#include <vector>



using namespace R12;
using namespace std;

ServiceID LinearSolver::pickService(){
	boost::random::uniform_int_distribution<ServiceID> distribution(0, instance().services().size() - 1);

	ServiceID service;
	uint32_t times = 0;
	const uint16_t MAX_TIMES = n_services;
	do{
		service = distribution(randomizer);
		++times;
		if(times % 1000 == 0)
			cout<<times<<endl;
	} while(instance().processesByService(service).size() <= 1 && times <= MAX_TIMES);
	
	return service;
}


void LinearSolver::run() {
	randomizer.seed(seed());

	
	n_processes = instance().processes().size();
	n_machines = instance().machines().size();
	n_resources = instance().resources().size();
	n_services = instance().services().size();
	n_neighborhoods = instance().neighborhoodCount();
	n_locations = instance().locationCount();
	n_balanceCosts = instance().balanceCosts().size();


#ifdef LS_DEBUG	
	cout << "Linear solver started" << endl;
	cout << "Processes: " << n_processes << endl;
	cout << "Machines: " << n_machines << endl;
	cout << "Resources: " << n_resources << endl;
	cout << "Services: " << n_services << endl;
	cout << "Locations: " << n_locations << endl;
	cout << "Balance costs: " << n_balanceCosts << endl;
#endif

	ServiceID service = pickService();
	LinearSolverSubProblem subProblem(instance(), initial(), service);
	
	subProblem.solve();
	
	//pool().push(sol.objective(), sol.solution());
	
	//signalCompletion();



}

