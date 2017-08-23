#include "nodeArchitecture.h"

struct NodeArch* newNodes(NetArch* netA, int numNodes)
{
	srand(time(NULL)); 

	if(numNodes == 0)
	{
		numNodes = 100;
	}	

	struct NodeArch *nodeA = malloc(sizeof *nodeA); 

	for(int i=0;i<numNodes;i++)
	{
		int temp_rand = rand();
		float energy = fmod((float) temp_rand, netA->energy.init); //energy is some random value between 0 and energy.init

		nodeA->node[i].id = i;
		nodeA->node[i].x = rand() % netA->yard.width; 	
		nodeA->node[i].y = rand() % netA->yard.height; 
		nodeA->node[i].energy = netA->energy.init; 
		nodeA->node[i].clusterHead = -1;
	}

	nodeA->numNode = numNodes;

	return nodeA;
}

struct NodeArch* fillNodes(NetArch* netA, int numNodes)
{
	srand(time(NULL)); 

	struct NodeArch *nodeA = malloc(sizeof *nodeA); 

	for(int i=0;i<numNodes;i++)
	{
		nodeA->node[i].id = i;
		nodeA->node[i].x = loc_xs[i]; 	
		nodeA->node[i].y = loc_ys[i]; 
		nodeA->node[i].energy = energies[i]; 
		nodeA->node[i].clusterHead = -1;
	}

	nodeA->numNode = numNodes;

	return nodeA;
}

