#pragma once

#include <map>

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
		-1 : Node already visited
		 0 : Node can be visited
		>0 : Node still depends on n other nodes
	*/
	std::vector<int> allowed_nodes;
	Route route;
};

class AntOptimizer {
private:
	/*
		Calculates unnormalized probability that `ant` will advance to `node`
		Returns 0 if `ant` is not allowed to visit `node`

		Numerator of formula (7.17) in [1]
	*/
	float edge_value(const Ant& ant, graph::Node node) const;

	/*
		Chooses between possible next nodes of `ant`
		and advances `ant` to chosen node.
	*/
	void advance_ant(Ant& ant);

	/*
		Calculates length (sum of weights) of visiting nodes in
		order of `route`
	*/
	int route_length(const std::vector<graph::Node>& route) const;

	/*
		Returns pheromone trail that `ant` leavs on `edge`
		Expects to only be called with edges `ant` actually visited
	*/
	float pheromone_update(const Ant& ant, graph::Edge edge) const;
	
	const float alpha; // Affects pheromone weight
	const float beta;  // Affects visibility weight
	const float roh;   // Evaporation of pheromone
	const float Q;     // Ant pheromone amount
	
	const graph::DirectedGraph& graph;
	const graph::DirectedGraph& sequence_graph;
	std::map<graph::Edge, int> edge_weight;
	std::map<graph::Edge, float> edge_pheromone;
	std::vector<Ant> initial_ants;
public:
	int round = 0;
	Route best_route;

	AntOptimizer(
		const graph::DirectedGraph& graph,
		const graph::DirectedGraph& sequence_graph,
		std::map<graph::Edge, int>& edge_weight,
		std::vector<Ant>& initial_ants,
		float initial_pheromone,
		float alpha,
		float beta,
		float roh,
		float q);

	float pheromone(graph::Edge edge) const;
	float max_pheromone() const;
	const std::map<graph::Edge, float>& pheromone_list() const;

	void optimize();
};

