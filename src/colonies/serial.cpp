#include <cmath>
#include <chrono>

#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif

#include "serial.hpp"

/*
	Optimize algorithm as described by [1]
*/
void SerialAntOptimizer::optimize() {
	// Init Ants
	std::vector<Ant> ants = initial_ants;
	const Ant* best_ant = nullptr;
	

	// 97% of function time is spent in this loop
	for (Ant& ant : ants) {
		ant.generator.seed(rand_device());
		
		// Let ants wander (96% of the loop body happens here)
		for (int i = 0; i < graph.node_count() - 1; i++) {
			advance_ant(ant);
			if (ant.current_node == graph::NO_NODE) { break; }
		}

		if (!goal_reached(ant)) {
			// Invalid solution
			continue;
		}

		ant.route.length = route_length(ant.route.nodes);
		update_best_route(ant);

		if (best_ant == nullptr || ant.route.length < best_ant->route.length) {
			best_ant = &ant;
		}
	}

	/*
		We don't need to initialize this.
		We call the `[]`-operator, which creates entries with the default initializer
		The default initializer for float returns 0.0, which is what we would initialize this to.
	*/
	std::map<graph::Edge, float> delta_pheromone;

	if (best_ant == nullptr) return;
	for (auto it = std::next(best_ant->route.nodes.begin()); it != best_ant->route.nodes.end(); it++) {
		graph::Edge edge(*std::prev(it), *it);
		delta_pheromone[edge] += pheromone_update(*best_ant, edge);
	}

	for (auto& edge_pair : edge_pheromone) {
		update_edge_pheromone(edge_pair.second, delta_pheromone.count(edge_pair.first) > 0 ? delta_pheromone.at(edge_pair.first) : 0);
	}
	
	round++;
}

void SerialAntOptimizer::optimize(int rounds) {
	while (rounds-- > 0) {
		optimize();
	}
}



