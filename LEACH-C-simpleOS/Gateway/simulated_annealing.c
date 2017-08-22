#include "simulated_annealing.h"


void simulated_annealing(int k){

	// generate a set of CHs randomly
	chooseCHs(bestClusterM);

	// assign nodes to CHs
	assignNodes(bestClusterM);

	// calculate energy requirements (fitness function)
	float bestEnergy = totalEnergy(bestClusterM);
	float energy;
	double expVal;
	double randVal;

	// create a temperature variable, cooling rate
	float temp = 10000;
	int round = 0;
	float coolingRate = 0.003;

	// LOOP until stop condition met:
	while(temp > 1){
		round++;

		copyClusterModel(newClusterM, bestClusterM);

		// select neighbour by making small change to current solution 
		// (i.e. swap out one CH for another random one, then reassign all nodes)

		modifyaCH(newClusterM);
		// to here
		assignNodes(newClusterM);

		// calculate energy requirements
		energy = totalEnergy(newClusterM);
		float energyB = totalEnergy(bestClusterM);

		// decide whether to move to that solution:
		
		// if neighbour solution is better than current solution
		expVal = exp((bestEnergy - energy) / temp);
		randVal = randZeroAndOne();
		if(energy < bestEnergy){
			//accept it
			bestClusterM = newClusterM;
			bestEnergy = energy;
		}
		else if(expVal > randVal){
			//	else if temperature is high enough, and neighbour solution is not too bad, accept it
			bestClusterM = newClusterM;
			bestEnergy = energy;
		}
		//else keep the old solution

		printf("%d %f\r\n", round, bestEnergy);
		// decrease temperature
		temp *= 1-coolingRate;
	}
}