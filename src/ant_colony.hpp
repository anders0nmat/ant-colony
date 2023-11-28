#pragma once

#include <vector>
#include <limits>

#include "undirected_graph.hpp"

class Ant {
public:
	int current_node;

	graph::UndirectedGraph* graph;
	graph::UndirectedGraph* order;

	std::vector<size_t> node_order;
	std::vector<bool> visited_nodes;
	std::vector<bool> allowed_nodes;

	void step();
};

int select_node(graph::AdjacencyEntry<std::tuple<>>);

int route_length(graph::UndirectedGraph*, std::vector<size_t>);

float pheromone_update(Ant&, std::pair<int, int>);

void ant_optimize(graph::UndirectedGraph* graph, int num_ants, graph::UndirectedGraph* dependencies) {
	const float PHEROMONE_TRACE = 0.1;

	std::vector<Ant> ants(num_ants);
	for (auto& ant : ants) {
		int city = 0;
		ant.current_node = city;
		ant.visited_nodes.push_back(city);
	}

	for (int i = 0; i < graph->node_count() - 1; i++) {
		for (auto& ant : ants) {
			graph::AdjacencyEntry neighbor_cities = graph->adjacency_list[ant.current_node];
			int next_node = select_node(neighbor_cities);
			ant.visited_nodes.push_back(next_node);
		}
	}

	std::vector<size_t> best_route;
	int best_length = std::numeric_limits<int>::max();

	std::map<std::pair<int, int>, float> delta_pheromone;
	std::map<std::pair<int, int>, float> pheromone;

	for (auto& ant : ants) {
		if (ant.visited_nodes.size() < graph->node_count() || !graph->has_edge(ant.node_order.front(), ant.node_order.back())) {
			// Ant did not visit all nodes
			continue;
		}
		ant.current_node = ant.visited_nodes[0];
		int length = route_length(graph, ant.node_order);
		if (length < best_length) {
			best_route = ant.node_order;
		}

		for (auto it = std::next(ant.node_order.begin()); it != ant.node_order.end(); it++) {
			std::pair<int, int> edge = std::minmax(*std::prev(it), *it);
			delta_pheromone[edge] += pheromone_update(ant, edge);
		}
	}

	for (auto& ph : pheromone) {
		ph.second *= 0.9; // Evaporation
		auto it = delta_pheromone.find(ph.first);
		if (it != delta_pheromone.end()) {
			ph.second += it->second;
		}
	}
}

