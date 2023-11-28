#pragma once

namespace graph {
	#include <tuple>
	#include <cstdint>
	#include <map>
	#include <memory>
	#include <vector>

	using Node = uint64_t;
	using Edge = std::pair<Node, Node>;

	template<typename Data>
	struct EdgeData {
		Edge edge;
		Data data;

		EdgeData(Edge e, Data d) :
			edge(e),
			data(d)
		{}
	};

	using Empty = std::tuple<>;

	template<typename E = Empty>
	using AdjacencyEntry = std::map<Node, EdgeData<E>*>;

	template<typename E = Empty>
	using AdjacencyList = std::vector<AdjacencyEntry<E>>;

	template<typename E = Empty>
	using EdgeList = std::vector<std::unique_ptr<EdgeData<E>>>;
}

