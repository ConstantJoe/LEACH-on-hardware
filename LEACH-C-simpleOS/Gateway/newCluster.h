/*
 *  Implementation of Mohammad Hossein Homaei's LEACH simulator, in C.
 *  Joseph Finnegan
 *  joseph.finnegan@cs.nuim.ie
 *  2017
 */

#include "clusterModel.h"
#include <math.h>
#include <stdio.h>

#ifndef NEWCLUSTER_H
#define NEWCLUSTER_H

double clusterOptimum(NetArch* netA, NodeArch* nodeA, double dBS);

ClusterModel* newClusterModel(NetArch* netA, NodeArch* nodeA);

ClusterModel* copyClusterModel(ClusterModel* to, ClusterModel* from);

void chooseCHs(ClusterModel* clusterM);

void assignNodes(ClusterModel* clusterM);

void clearCHs(ClusterModel* clusterM);

void modifyaCH(ClusterModel* clusterM);

void printNetDetails(ClusterModel* clusterM);

void findClosestNodes(ClusterModel* clusterM);

#endif /* NEWCLUSTER_H */
