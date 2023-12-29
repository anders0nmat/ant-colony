#pragma once

#include <cstdint>
#include <vector>
#include <set>
#include <algorithm>

namespace graph {

	using Node = int32_t;
	using Edge = std::pair<Node, Node>;

	const Node NO_NODE = -1;

	using NodeList = std::set<Node>;
	using EdgeList = std::set<Edge>;

	using AdjacencyEntry = std::set<Node>;

	using AdjacencyList = std::vector<AdjacencyEntry>;

	template<bool Directed>
	class Graph {
	private:
		Edge minmax(Edge p) const {
			// Only enforce order if graph is undirected
			return Directed ? p : static_cast<Edge>(::std::minmax(p.first, p.second));
		}
	public:
		EdgeList edges;
		AdjacencyList adjacency_list;

		Graph(int nodes = 0) :
			edges(),
			adjacency_list(nodes)
		{}

		size_t node_count() const {
			return adjacency_list.size();
		}

		bool has_node(Node node) const {
			return node >= 0 && node < adjacency_list.size();
		}

		void add_node() {
			adjacency_list.push_back(AdjacencyEntry());
		}

		void remove_node(Node node) {
			if (!has_node(node)) { return; }

			// Delete all edges to `node`
			for (auto first = edges.begin(), last = edges.end(); first != last;) {
				if (first->first == node || first->second == node) {
					first = edges.erase(first);
				}
				else {
					first++;
				}
			}
			// Delete `node` and all entries of it
			adjacency_list.erase(::std::next(adjacency_list.begin(), node));
			for (AdjacencyEntry& nodeMap : adjacency_list) {
				nodeMap.erase(node);
			}
		}

		size_t edge_count() const {
			return edges.size();
		}

		bool has_edge(Edge edge) const {
			return edges.count(edge) > 0;
		}

		bool has_edge(Node from, Node to) const {
			return has_edge(::std::make_pair(from, to));
		}

		void add_edge(Edge edge) {
			if (!has_node(edge.first) || !has_node(edge.second)) { return; }
			if (!has_edge(edge)) {
				edges.insert(minmax(edge));
				adjacency_list[edge.first].insert(edge.second);
				if (!Directed) {
					adjacency_list[edge.second].insert(edge.first);
				}
			}
		}

		void add_edge(Node from, Node to) {
			add_edge(::std::make_pair(from, to));
		}

		void remove_edge(Edge edge) {
			if (!has_edge(edge)) { return; }
			adjacency_list[edge.first].erase(edge.second);
			if (!Directed) {
				adjacency_list[edge.second].erase(edge.first);
			}
			edges.erase(minmax(edge));
		}

		void remove_edge(Node from, Node to) {
			remove_edge(::std::make_pair(from, to));
		}

		Graph<Directed> inverted() const {
			if (!Directed) { return *this; }

			Graph<Directed> invertedGraph(node_count());
			for (Edge e : edges) {
				::std::swap(e.first, e.second);
				invertedGraph.add_edge(e);
			}

			return invertedGraph;
		}
	};

	using UndirectedGraph = Graph<false>;
	using DirectedGraph = Graph<true>;
}

