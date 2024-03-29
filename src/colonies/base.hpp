#pragma once

#include <map>
#include <random>
#include <chrono>
#include <algorithm>

#include "../graph.hpp"

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

	std::default_random_engine generator;
};

struct Parameters {
	float alpha;
	float beta;
	float roh;
	float q;
	float initial_pheromone;
	float min_pheromone;
	float max_pheromone;
	float zero_distance;
};

struct Profiler {
	typedef std::chrono::high_resolution_clock Clock;	
	typedef Clock::duration Duration;
	typedef Clock::time_point Timepoint;
	
	std::vector<Duration> durations;

	Timepoint start_point;

	void start() {
		start_point = Clock::now();
	}

	void stop() {
		auto elapsed = Clock::now() - start_point;
		durations.push_back(elapsed);
	}

	Duration total() const {
		return std::accumulate(durations.begin(), durations.end(), Duration());
	}

	Duration avg() const {
		return total() / durations.size();
	}

	std::pair<Duration, Duration> min_max() const {
		const auto mm = std::minmax_element(durations.begin(), durations.end());
		return std::make_pair(*mm.first, *mm.second);
	}
};

class AntOptimizer {
protected:
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
	void advance_ant(Ant& ant) const;

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

	/*
		Saves the best route so far, updating if neccessary.
		Return value indicates whether ant had a better route.
	*/
	bool update_best_route(const Ant& ant);

	/*
		Responsible for updating an edge given its value and calculated delta
	*/
	void update_edge_pheromone(float& value, const float delta);

	/*
		Checks whether an ant has reached its goal and can be considered
		in future analysis.
	*/
	bool goal_reached(const Ant& ant) const;
	
	
	const graph::DirectedGraph& graph;
	const graph::DirectedGraph& sequence_graph;
	const std::map<graph::Edge, int> edge_weight;
	std::map<graph::Edge, float> edge_visibility;
	std::map<graph::Edge, float> edge_pheromone;
	std::vector<Ant> initial_ants;
	std::random_device rand_device;
public:
	const Parameters params;
	int round = 0;
	Route best_route;
	std::string init_args;

	AntOptimizer(
		const graph::DirectedGraph& graph,
		const graph::DirectedGraph& sequence_graph,
		const std::map<graph::Edge, int>& edge_weight,
		const std::vector<Ant>& initial_ants,
		Parameters params);

	virtual ~AntOptimizer() = default;

	float pheromone(graph::Edge edge) const;
	std::pair<float, float> minmax_pheromone() const;
	const std::map<graph::Edge, float>& pheromone_list() const;

	virtual void init(std::string args) {}

	virtual void optimize() {}
	virtual Profiler optimize(int rounds) { return Profiler(); }

	static constexpr const char* _name = "abstract";
	virtual std::string name() { return _name; }
};



