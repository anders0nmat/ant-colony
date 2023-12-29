#pragma once

#include <vector>
#include <limits>
#include <map>
#include <cmath>
#include <random>

#include <iostream>

#include "graph.hpp"

struct Route {
	std::vector<graph::Node> nodes;
	int length;

	Route(int length = -1) : nodes({}), length(length) {}
};

class Ant {
public:
	Ant(int current_node) 
	: current_node(current_node), allowed_nodes(), route(Route()) {}

	int current_node;

	/*
		Represents how many nodes need to be visited before node i
	*/
	std::vector<int> allowed_nodes;
	Route route;

	std::string print() {
		std::string result = "Ant\n";
		result += "  current_node=" + std::to_string(current_node) + "\n";
		result += "  allowed_nodes=[";
		if (!allowed_nodes.empty()) {
			for (auto i : allowed_nodes) {
				result += "\n    " + std::to_string(i) + ",";
			}
		}
		result += "]\n";
		return result;
	}
};

class AntOptimizer {
public:
	float alpha; // Affects pheromone weight
	float beta;  // Affects visibility weight
	float roh;   // Evaporation of pheromone
	float Q;     // Ant pheromone amount
private:
	/*
		Calculates unnormalized probability that `ant` will advance to `node`
		Returns 0 if `ant` is not allowed to visit `node`

		Numerator of formula (7.17) in [1]
	*/
	float edge_value(const Ant& ant, graph::Node node) {
		if (ant.allowed_nodes[node] != 0) { return 0; }

		float pher = edge_pheromone.at(graph::Edge(ant.current_node, node));
		float len  = edge_weight   .at(graph::Edge(ant.current_node, node));
		float vis  = 1 / (len == 0 ? 1E-6 : len);
		return std::pow(pher, alpha) * std::pow(vis, beta);
	}

	void advance_ant(Ant& ant) {
		if (ant.current_node == graph::NO_NODE) { return; }
		
		/*
			`choices` is a collection of ranges all defined by their upper bound
			represented by a pair of (upper bound, choice)

			    0_____|____|________x
			       r1   r2     r3

			`rand` = [0, x)
			`next` = first r where rand < upper_bound
		*/
		std::vector<std::pair<float, graph::Node>> choices;
		float sum = 0;
		for (const graph::Node node : graph.adjacency_list[ant.current_node]) {
			float value = edge_value(ant, node);
			choices.emplace_back(sum + value, node);
			sum += value;
		}
		std::random_device rd;
		std::default_random_engine generator(rd());
		std::uniform_real_distribution<float> distribution(0.0, sum);
		float rand = distribution(generator);
		
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
			Fun things to spend a thursdy night on...

			Lesson learned: Use std::vector::at() whenever possible. It correctly throws an out_of_bounds
			exception.
		*/

		// Mark this node as visited
		ant.allowed_nodes.at(next) = -1;

		// Update dependent nodes
		for (const graph::Node node : sequence_graph.adjacency_list[next]) {
			ant.allowed_nodes[node] -= 1;
		}
	}

	int route_length(const std::vector<graph::Node>& route) {
		int total = 0;
		for (auto it1 = route.begin(), it2 = std::next(it1); it2 != route.end(); it1++, it2++) {
			auto entry = edge_weight.find(graph::Edge(*it1, *it2));
			if (entry == edge_weight.end()) { return std::numeric_limits<int>::max(); }
			total += entry->second;
		}
		return total;
	}

	float pheromone_update(const Ant& ant, graph::Edge edge) {
		float L_k = ant.route.length;
		
		return Q / L_k;
	}

public:
	
	const graph::DirectedGraph& graph;
	const graph::DirectedGraph& sequence_graph;
	std::map<graph::Edge, int> edge_weight;
	std::map<graph::Edge, float> edge_pheromone;
	float _max_pheromone;
	int round = 0;
	std::vector<Ant> initial_ants;

	Route best_route;

	float max_pheromone() {
		if (_max_pheromone == -1) {
			for (const auto& ph : edge_pheromone) {
				_max_pheromone = std::max(_max_pheromone, ph.second);
			}
		}

		return _max_pheromone;
	}

	AntOptimizer(
		const graph::DirectedGraph& graph,
		const graph::DirectedGraph& sequence_graph,
		std::map<graph::Edge, int>& edge_weight,
		std::vector<Ant>& initial_ants,
		float initial_pheromone,
		float alpha,
		float beta,
		float roh,
		float q)
	
	: alpha(alpha), beta(beta), roh(roh), Q(q), graph(graph), sequence_graph(sequence_graph), edge_weight(edge_weight), initial_ants(initial_ants) {
		
		for (const auto& edge : graph.edges) {
			edge_pheromone.emplace(edge, initial_pheromone);
		}
		_max_pheromone = initial_pheromone;

		best_route = Route(std::numeric_limits<int>::max());

		// build allowed_list for ants
		std::vector<int> allowed_list(graph.node_count());
		graph::DirectedGraph reverse_seq = sequence_graph.inverted();
		for (graph::Node node = 0; node < reverse_seq.node_count(); node++) {
			allowed_list[node] = reverse_seq.adjacency_list[node].size();
		}

		// mark start as visited
		for (Ant& ant : this->initial_ants) {
			ant.allowed_nodes = allowed_list;
			ant.allowed_nodes[ant.current_node] = -1;

			for (const graph::Node node : sequence_graph.adjacency_list[ant.current_node]) {
				ant.allowed_nodes[node] -= 1;
			}
		}
	}

	void optimize() {
		// Init Ants
		std::vector<Ant> ants = initial_ants;
		_max_pheromone = -1;

		// Let ants wander
		for (int i = 0; i < graph.node_count() - 1; i++) {
			for (size_t idx = 0; idx < ants.size(); idx++) {
				advance_ant(ants[idx]);
			}
		}

		/*
			We don't need to initialize this.
			We call the `[]`-operator, which creates entries with the default initializer
			The default initializer for float returns 0.0, which is what we would initialize this to.
		*/
		std::map<graph::Edge, float> delta_pheromone;
		
		for (Ant& ant : ants) {
			if (ant.current_node == graph::NO_NODE || !graph.has_edge(ant.route.nodes.front(), ant.route.nodes.back())) {
				// Invalid solution
				continue;
			}

			ant.route.length = route_length(ant.route.nodes);
			if (ant.route.length < best_route.length) {
				best_route = ant.route;
			}

			for (auto it = std::next(ant.route.nodes.begin()); it != ant.route.nodes.end(); it++) {
				graph::Edge edge(*std::prev(it), *it);
				delta_pheromone[edge] += pheromone_update(ant, edge);
			}
		}

		// foreach pheromone { pheromone *= evaporation; pheromone += delta_pheromone}
		for (auto it = edge_pheromone.begin(); it != edge_pheromone.end(); it++) {
			// (7.14) in [1]
			it->second *= (1.0f - roh);
			it->second += delta_pheromone.count(it->first) > 0 ? delta_pheromone[it->first] : 0.0f;
		}

		round++;
	}
};

