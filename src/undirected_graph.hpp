#pragma once

#include <vector>
#include <memory>
#include <map>
#include <algorithm>

#include "graph.hpp"

namespace graph {

	template<typename E = Empty>
	class UndirectedGraph {
	private:
		Edge minmax(Edge p) {
			return std::minmax(p.first, p.second);
		}
	public:
		EdgeList<E> edges;
		AdjacencyList<E> adjacency_list;

		size_t node_count() {
			return adjacency_list.size();
		}

		bool has_node(Node node) {
			return node >= 0 && node < adjacency_list.size();
		}

		Node add_node() {
			adjacency_list.push_back(AdjacencyList<E>());
			return adjacency_list.size() - 1;
		}

		void remove_node(Node node) {
			if (!has_node(node)) { return; }

			// Delete all edges to `node`
			auto newEndIt = std::remove_if(edges.begin(), edges.end(), 
				[node](const std::unique_ptr<EdgeData<E>>& e){
					return e->edge.first == node || e->edge.second == node;
				});
			edges.resize(std::distance(edges.begin(), newEndIt));

			// Delete `node` and all entries of it
			adjacency_list.erase(std::next(adjacency_list.begin(), node));
			for (AdjacencyEntry<E>& nodeMap : adjacency_list) {
				nodeMap.erase(node);
			}
		}

		size_t edge_count() {
			return edges.size();
		}

		bool has_edge(Edge edge) {
			return get_edge(edge) != nullptr;
		}

		bool has_edge(Node from, Node to) {
			return get_edge(from, to) != nullptr;
		}

		Edge* get_edge(Edge edge) {
			if (!has_node(edge.first) || !has_node(edge.second)) { return nullptr; }
			const std::map<Node, Edge*>& al = adjacency_list[edge.first];
			auto it = al.find(edge.second);
			return it != al.end() ? it->second : nullptr;
		}

		Edge* get_edge(Node from, Node to) {
			return get_edge(std::make_pair(from, to));
		}

		Edge* add_edge(Edge edge, E data = E()) {
			if (!has_node(edge.first) || !has_node(edge.second)) { return nullptr; }
			if (!has_edge(edge)) {
				edges.push_back(std::make_unique<EdgeData<E>>(minmax(edge), data));
				Edge* e = edges.back().get();
				adjacency_list[edge.first].emplace(edge.second, e);
				adjacency_list[edge.second].emplace(edge.first, e);
				return e;
			}
			return nullptr;
		}

		Edge* add_edge(Node from, Node to, E data = E()) {
			return add_edge(std::make_pair(from, to), data);
		}

		void remove_edge(Edge edge) {
			if (!has_edge(edge)) { return; }
			Edge* e = adjacency_list[edge.first].at(edge.second);
			adjacency_list[edge.first].erase(edge.second);
			adjacency_list[edge.second].erase(edge.first);
			edges.erase(std::find_if(edges.begin(), edges.end(), [e](const std::unique_ptr<Edge>& pe){ return pe.get() == e; }));
			// e = nullptr; // e does no longer exist
		}

		void remove_edge(Node from, Node to) {
			remove_edge(std::make_pair(from, to));
		}
	};
}

