#include <cmath>
#include <chrono>
#include <pthread.h>

#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif

#include "parallel_ant_colony.hpp"

struct ThreadArgs {
	Ant& ant;
	const ParallelAntOptimizer& optimizer;

	ThreadArgs(Ant& a, const ParallelAntOptimizer& o) : ant(a), optimizer(o) {}
};

void* optimize_threaded(void* __args) {
	ThreadArgs* args = static_cast<ThreadArgs*>(__args);

	// Let ants wander (96% of the loop body happens here)
	for (int i = 0; i < args->optimizer.graph.node_count() - 1; i++) {
		args->optimizer.advance_ant(args->ant);
		if (args->ant.current_node == graph::NO_NODE) { break; }
	}

	if (!args->optimizer.goal_reached(args->ant)) {
		// Invalid solution
		return nullptr;
	}

	args->ant.route.length = args->optimizer.route_length(args->ant.route.nodes);
	return nullptr;
}


/*
	Optimize algorithm as described by [1]
	with pthread applied
*/
void ParallelAntOptimizer::optimize() {
	// Init Ants
	std::vector<Ant> ants = initial_ants;
	const Ant* best_ant = nullptr;
	
	std::vector<pthread_t> threads;
	threads.reserve(ants.size());
	std::vector<ThreadArgs> thread_args;
	thread_args.reserve(ants.size());

	for (Ant& ant : ants) {
		ant.generator.seed(rand_device());

		thread_args.emplace_back(ant, *this);
		threads.emplace_back(0);
		int succ = pthread_create(&threads.back(), nullptr, optimize_threaded, static_cast<void*>(&thread_args.back()));
		if (succ != 0) {
			std::cout << "pthread_create returned " << succ << std::endl;
			exit(1);
		}
	}

	for (size_t i = 0; i < threads.size(); i++) {
		int succ = pthread_join(threads.at(i), nullptr);
		if (succ != 0) {
			std::cout << "pthread_join returned " << succ << std::endl;
			exit(1);
		}

		const Ant& ant = ants.at(i);

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

void ParallelAntOptimizer::optimize(int rounds) {
	while (rounds-- > 0) {
		optimize();
	}
}



