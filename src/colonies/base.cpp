#include <cmath>
#include <chrono>

#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif

#include "base.hpp"

float AntOptimizer::edge_value(const Ant& ant, graph::Node node) const {
	if (ant.allowed_nodes.at(node) != 0) { return 0; }

	float pher = edge_pheromone .at(graph::Edge(ant.current_node, node));
	float vis  = edge_visibility.at(graph::Edge(ant.current_node, node));
	return std::pow(pher, params.alpha) * vis;

	// vis is precalculated in edge_visibility
	//float len  = edge_weight   .at(graph::Edge(ant.current_node, node));
	//float vis  = 1 / (len == 0 ? params.zero_distance : len);
	//vis = std::pow(vis, params.beta);
}

void AntOptimizer::advance_ant(Ant& ant) const {
	if (ant.current_node == graph::NO_NODE) { return; }
	
	/*
		`choices` is a collection of ranges all defined by their upper bound
		represented by a pair of (upper bound, choice)

			0_____|____|________x
			   r1   r2     r3

		`rand` = [0, x)
		`next` = first r where rand < upper_bound

		Used to easily select choice without normalizing first
	*/
	std::vector<std::pair<float, graph::Node>> choices;
	choices.reserve(graph.node_count());

	float sum = 0;
	// ~90% of function is spent in this loop (without otimizations)
	for (const graph::Node node : graph.adjacency_list.at(ant.current_node)) {
		float value = edge_value(ant, node);
		sum += value;
		choices.emplace_back(sum, node);
	}
	
	std::uniform_real_distribution<float> distribution(0.0, sum);
	float rand = distribution(ant.generator);
	
	graph::Node next = graph::NO_NODE;
	for (const auto& pair : choices) {
		if (rand < pair.first) {
			next = pair.second;
			break;
		}
	}

	ant.current_node = next;
	ant.route.nodes.push_back(next);

	if (next < 0) { return; }
	/*
		WTF? std::vector::operator[] does not check bounds.
		Instead it unleashed undefined behavior if you try to access
		an illegal index.
		In this case, if there are no choices left (because the pheromones evaporated completely)
		I've defaulted to -1 to indicate "No next node possible". However, operator[] takes
		a size_t aka uint, so my -1 gets silently converted to size_t::max(). And the 
		allowed_nodes vector is indeed not some 1 quadrilion elements long.

		But this illegal access manifests in a free() exception some 200 iterations in the algorithm.
		Fun things to spend a thursday night on...

		Lesson learned: Use std::vector::at() whenever possible. It correctly throws an out_of_bounds
		exception.
	*/

	// Mark this node as visited
	ant.allowed_nodes.at(next) = -1;

	// Update dependent nodes
	for (const graph::Node node : sequence_graph.adjacency_list.at(next)) {
		ant.allowed_nodes.at(node) -= 1;
	}
}

int AntOptimizer::route_length(const std::vector<graph::Node>& route) const {
	int total = 0;
	for (auto it1 = route.begin(), it2 = std::next(it1); it2 != route.end(); it1++, it2++) {
		auto entry = edge_weight.find(graph::Edge(*it1, *it2));
		if (entry == edge_weight.end()) { return std::numeric_limits<int>::max(); }
		total += entry->second;
	}
	return total;
}

float AntOptimizer::pheromone_update(const Ant& ant, graph::Edge edge) const {
	float L_k = ant.route.length;
	
	return params.q / L_k;
}

std::pair<float, float> AntOptimizer::minmax_pheromone() const {
	std::pair<float, float> minmax = std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
	for (const auto& ph : edge_pheromone) {
		minmax.first = std::min(minmax.first, ph.second);
		minmax.second = std::max(minmax.second, ph.second);
	}
	return minmax;
}

bool AntOptimizer::update_best_route(const Ant& ant) {
	if (ant.route.length < best_route.length) {
		best_route = ant.route;
		return true;
	}
	return false;
}

void AntOptimizer::update_edge_pheromone(float& value, const float delta) {
	value *= (1.0f - params.roh);
	value += delta;
	value = std::clamp(value, params.min_pheromone, params.max_pheromone);
}

bool AntOptimizer::goal_reached(const Ant& ant) const {
	bool 
		ant_lost = ant.current_node == graph::NO_NODE,
		not_at_end = ant.current_node != graph.node_count() - 1;
	
	return !(ant_lost || not_at_end);
}


AntOptimizer::AntOptimizer(
	const graph::DirectedGraph& graph,
	const graph::DirectedGraph& sequence_graph,
	const std::map<graph::Edge, int>& edge_weight,
	const std::vector<Ant>& initial_ants,
	Parameters params)

: graph(graph),
  sequence_graph(sequence_graph), edge_weight(edge_weight), initial_ants(initial_ants), params(params) {
	
	for (const auto& edge : graph.edges) {
		edge_pheromone.emplace(edge, params.initial_pheromone);
	}

	best_route = Route(std::numeric_limits<int>::max());

	// build allowed_list for ants
	std::vector<int> allowed_list(graph.node_count());
	graph::DirectedGraph reverse_seq = sequence_graph.inverted();
	for (graph::Node node = 0; node < reverse_seq.node_count(); node++) {
		allowed_list.at(node) = reverse_seq.adjacency_list.at(node).size();
	}

	// mark start as visited
	for (Ant& ant : this->initial_ants) {
		ant.allowed_nodes = allowed_list;
		ant.allowed_nodes.at(ant.current_node) = -1;
		ant.route.nodes.push_back(ant.current_node);

		for (const graph::Node node : sequence_graph.adjacency_list.at(ant.current_node)) {
			ant.allowed_nodes.at(node) -= 1;
		}
	}

	// Precalculate Visibility into edge_weights
	for (auto & p : edge_weight) {
		this->edge_visibility.emplace(p.first, std::pow(1 / std::max(static_cast<float>(p.second), params.zero_distance), params.beta));
	}
}

float AntOptimizer::pheromone(graph::Edge edge) const {
	return edge_pheromone.count(edge) > 0 ? edge_pheromone.at(edge) : 0;
}

const std::map<graph::Edge, float>& AntOptimizer::pheromone_list() const {
	return edge_pheromone;
}



