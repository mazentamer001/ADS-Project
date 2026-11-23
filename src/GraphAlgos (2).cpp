
#ifndef GRAPHALGOS_HPP
#define GRAPHALGOS_HPP

#include "Graph.hpp"
#include "DynArray.hpp"


bool levelize(const Graph& g, DynArray<int>& topoOrder, DynArray<int>& levels);


bool findLoop(const Graph& g, DynArray<int>& cycleGates);

#endif