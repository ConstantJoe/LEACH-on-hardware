/*
 *  Implementation of Mohammad Hossein Homaei's LEACH simulator, in C.
 *  Joseph Finnegan
 *  joseph.finnegan@cs.nuim.ie
 *  2017
 */

#include "newCluster.h"

/*
 *	Calculates the optimal number of clusters based on the area size, number of nodes, and communication energy models.
 *	See "An Application-Specific Protocol Architecture for Wireless Microsensor Networks" by Heinzelman et. al. for a 
 *	derivation of the formula.
 */
double clusterOptimum(NetArch* netA, NodeArch* nodeA, double dBS)
{
	int n = nodeA->numNode;
	double m = sqrt(netA->yard.height * netA->yard.width);

	float x = netA->energy.freespace / netA->energy.multipath;

	double kopt = sqrt(n) / sqrt(2*M_PI) * sqrt(netA->energy.freespace / netA->energy.multipath) * m / pow(dBS,2);
	kopt = round(kopt);
	return kopt;
}

/**
*	Creates a "blank" cluster model to use in this round of operation, populated with just information on the environment and 
*	individual node locations and energies. Creation of clusters from this is handled in the clusterRun function.
**/
ClusterModel* newClusterModel(NetArch* netA, NodeArch* nodeA)
{
	struct ClusterModel *clusterM = malloc(sizeof *clusterM); 
	clusterM->netA = *netA;
	clusterM->nodeA = *nodeA;

	return clusterM;
}

void printNetDetails(ClusterModel* clusterM){
	for(int i=0; i<clusterM->nodeA.numNode; i++)
	{
		printf("ch: %d\r\n", clusterM->nodeA.node[i].clusterHead);
		printf("id: %d\r\n", clusterM->nodeA.node[i].id);
		printf("energy: %f\r\n\r\n", clusterM->nodeA.node[i].energy);

		printf("closest node: %d\r\n", clusterM->nodeA.node[i].closestNodes[0]);
		printf("2nd closest node: %d\r\n", clusterM->nodeA.node[i].closestNodes[1]);
		printf("3rd closest node: %d\r\n", clusterM->nodeA.node[i].closestNodes[2]);
	}
}

/**
*	Calculates the optimal number of nodes, then calls the leach algorithm to generate clusters. Leach algorithm is defined separately
*	to enable this method to be easily modified to handle a different clustering algorithm.  
**/

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

void clearCHs(ClusterModel* clusterM){
	for(int i=0; i<clusterM->nodeA.numNode; i++)
	{
		if(!(clusterM->nodeA.node[i].clusterHead == clusterM->nodeA.node[i].id)){
			clusterM->nodeA.node[i].clusterHead = -1;	
		}
	}

	clusterM->clusterN.countCHs = 0;
}

void modifyaCH(ClusterModel* clusterM){

	// This method needs to be changed.
	//	It would be better to choose a CH at random, then choose one of its neighbours at random
	//	Repeat that x number of times, choose the best solution found

	//choose one of the CHs at random
	int temp_rand = rand(); 
	int choiceCH = temp_rand % clusterM->clusterN.countCHs;
		

	//set it to be a normal node again
	int choiceInd = clusterM->clusterN.cNodes[choiceCH].no;
	clusterM->nodeA.node[choiceInd].clusterHead = -1;

	//set a different random node to be a CH
	int stop = 1;
	
	while(stop){
		temp_rand = rand();
		int choice = temp_rand % clusterM->nodeA.numNode;
		
		if(!(clusterM->nodeA.node[choice].clusterHead == clusterM->nodeA.node[choice].id) && choice != choiceInd)
		{
			clusterM->nodeA.node[choice].clusterHead = clusterM->nodeA.node[choice].id;
			clusterM->clusterN.cNodes[choiceCH].no = choice; 
			clusterM->clusterN.cNodes[choiceCH].locX = clusterM->nodeA.node[choice].x; 
			clusterM->clusterN.cNodes[choiceCH].locY = clusterM->nodeA.node[choice].y;
			clusterM->clusterN.cNodes[choiceCH].distance = sqrt(pow(clusterM->nodeA.node[choice].x - clusterM->netA.sink.x, 2) + pow(clusterM->nodeA.node[choice].y - clusterM->netA.sink.y, 2));
		
			stop = 0;
		}	
	}
}

void assignNodes(ClusterModel* clusterM)
{
	for(int i=0; i<clusterM->nodeA.numNode; i++){
		if(!(clusterM->nodeA.node[i].clusterHead == clusterM->nodeA.node[i].id)) //if node is not a CH
		{
			int locNodeX = clusterM->nodeA.node[i].x;
       		int locNodeY = clusterM->nodeA.node[i].y;
        	int countCH = clusterM->clusterN.countCHs;

        	//get the shortest distance to a cluster head
        	int loc = 0;
        	double minDis = 1000000;
            
        	for(int j=0;j<countCH;j++)
        	{
            	double dist = sqrt( pow(locNodeX - clusterM->clusterN.cNodes[j].locX, 2) + pow(locNodeY - clusterM->clusterN.cNodes[j].locY, 2));
 
            	if(dist < minDis)
            	{
                	minDis = dist;
                	loc = j; 
            	}
        	}
        	clusterM->nodeA.node[i].clusterHead = clusterM->clusterN.cNodes[loc].no;
		}
	}
}

ClusterModel* copyClusterModel(ClusterModel* to, ClusterModel* from){ 
	to->netA = from->netA;
	to->nodeA = from->nodeA;
	to->clusterN = from->clusterN;
	to->numCluster = from->numCluster;
	to->p = from->p;
	return to;
}

void findClosestNodes(ClusterModel* clusterM){
	for(int i=0; i<clusterM->nodeA.numNode; i++)
	{
		int closest, secClosest, trdClosest;
		double closestDist, secClosestDist, trdClosestDist, tempDist;

		if(i==0){
			closest = 1;
			closestDist = sqrt(pow(clusterM->nodeA.node[i].x -  clusterM->nodeA.node[1].x, 2) + pow( clusterM->nodeA.node[i].y - clusterM->nodeA.node[1].y, 2));
		} else{
			closest = 0;
			closestDist = sqrt(pow(clusterM->nodeA.node[i].x -  clusterM->nodeA.node[0].x, 2) + pow( clusterM->nodeA.node[i].y - clusterM->nodeA.node[1].y, 2));
		}		
		 

		for(int j=0; j<clusterM->nodeA.numNode; j++)
		{
			if(i!=j){
				tempDist = sqrt(pow(clusterM->nodeA.node[i].x -  clusterM->nodeA.node[j].x, 2) + pow( clusterM->nodeA.node[i].y - clusterM->nodeA.node[j].y, 2));
				
				if(tempDist < closestDist){
					trdClosestDist = secClosestDist;
					trdClosest = secClosest;

					secClosestDist = closestDist;
					secClosest = closest;

					closestDist = tempDist;
					closest = j;
				} else if(tempDist < secClosestDist){
					trdClosestDist = secClosestDist;
					trdClosest = secClosest;

					secClosestDist = tempDist;
					secClosest = j;
				} else if(tempDist < trdClosestDist){
					trdClosestDist = tempDist;
					trdClosest = j;
				}
			}
		}
		clusterM->nodeA.node[i].closestNodes[0] = closest;
		clusterM->nodeA.node[i].closestNodes[1] = secClosest;
		clusterM->nodeA.node[i].closestNodes[2] = trdClosest;
	}
}