#include "netArchitecture.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef NODEARCHITECTURE_H
#define NODEARCHITECTURE_H

#define MAX_NODES 100

typedef struct Node {
	int id;
	int x;
	int y;
	float energy;
	int clusterHead;
	int closestNodes[3];
} Node;

typedef struct NodeArch {
	int numNode;
	Node node[MAX_NODES];
} NodeArch;

struct NodeArch* newNodes(NetArch* netA, int numNodes);

#endif /* NODEARCHITECTURE_H */

