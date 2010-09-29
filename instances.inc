#ifndef INSTANCES_INC
#define INSTANCES_INC

#include "graph.h"
#include "block.h"
#include "maxflow.h"

#ifdef _MSC_VER
#pragma warning(disable: 4661)
#endif

// Instantiations: <captype, tcaptype, flowtype>
// IMPORTANT: 
//    flowtype should be 'larger' than tcaptype 
//    tcaptype should be 'larger' than captype

template class Graph<int,int,int>;
template class Graph<short,int,int>;
template class Graph<float,float,float>;
template class Graph<double,double,double>;

template Graph<int,int,int>::Graph(int, int, void (*)(char *));
template Graph<int,int,int>::~Graph();

template Graph<double,double,double>::Graph(int, int, void (*)(char *));
template Graph<double,double,double>::~Graph();


//template int Graph<int,int,int>::maxflow(bool, Block<int>*);

#endif