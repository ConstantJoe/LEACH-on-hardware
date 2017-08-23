#include "simulated_annealing.h"

void chooseCHs(ClusterModel* clusterM)
{
	//choose numCluster cluster heads randomly
	int stop = 1;
	int countCHs = 0;
	int i;
	while(countCHs < clusterM->numCluster && stop){
		for(i=0; i<clusterM->nodeA.numNode; i++)
		{
			
			if(!(clusterM->nodeA.node[i].clusterHead == clusterM->nodeA.node[i].id)) //if not already a CH
			{
				int temp_rand = rand(); 
				
				if((temp_rand % clusterM->nodeA.numNode) <= clusterM->numCluster)
				{
					
					clusterM->nodeA.node[i].clusterHead = clusterM->nodeA.node[i].id;
					
					clusterM->clusterN.cNodes[countCHs].locX = clusterM->nodeA.node[i].x; 
					clusterM->clusterN.cNodes[countCHs].locY = clusterM->nodeA.node[i].y;
					clusterM->clusterN.cNodes[countCHs].distance = sqrt(pow(clusterM->nodeA.node[i].x - clusterM->netA.sink.x, 2) + pow(clusterM->nodeA.node[i].y - clusterM->netA.sink.y, 2));
					clusterM->clusterN.cNodes[countCHs].no = i; 
					countCHs++;



					if(countCHs == clusterM->numCluster){
						stop = 0;
						break;
					}
				}
			}
		}
	}

	clusterM->clusterN.countCHs = countCHs;
}

FormationPacket simulated_annealing(int k){

	//create an environment
	struct NetArch* netA  	 = newNetwork(0, 0, 0, 0);
	
	// generate a population of nodes (w/ randomish energy - we're assuming this is at the end of an iteration)
		//also assuming all nodes are alive 
	struct NodeArch* nodeA 	 = newNodes(netA, NUMNODES);

	// create a struct to hold all network information
	struct ClusterModel* bestClusterM = newClusterModel(netA, nodeA);
	findClosestNodes(bestClusterM);

	// generate optimal k, the number of CHs
	double dBS = sqrt(pow(bestClusterM->netA.sink.x - bestClusterM->netA.yard.height, 2) + pow(bestClusterM->netA.sink.y - bestClusterM->netA.yard.width, 2));
	bestClusterM->numCluster = clusterOptimum(&bestClusterM->netA, &bestClusterM->nodeA, dBS);

	// generate a set of CHs randomly
	chooseCHs(bestClusterM);

	// assign nodes to CHs
	assignNodes(bestClusterM);

	//printNetDetails(bestClusterM);

	// calculate energy requirements (fitness function)
	float bestEnergy = totalEnergy(bestClusterM);
	float energy;
	double expVal;
	double randVal;

	// create a temperature variable, cooling rate
	float temp = 10000;
	int round = 0;
	float coolingRate = 0.003;

	struct ClusterModel* newClusterM = malloc(sizeof *newClusterM);


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

	//return FormationPacket
	FormationPacket fp;
	fp.type = P_FORMATIONPACKET;
	fp.numNodes = NUMNODES;
	
	int i;
	for(i=0; i<NUMNODES; i++){
		fp.assignedCHs[i] = bestClusterM.nodeA.node[i].clusterHead;
	}
	return fp;
}