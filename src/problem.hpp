#include <fstream>
#include <string>
#include <map>

#include <iostream>

#include "graph.hpp"

bool read_key(std::string content, std::string key, std::string& value) {
	if (value.empty() && content.find(key) == 0) {
		auto p = content.find_first_not_of(": \t", key.size());
		value = content.substr(p);
		return true;
	}
	return false;
}

struct Problem {
	std::string name;
	std::string comment;

	graph::DirectedGraph graph;
	std::map<graph::Edge, int> weights;
	graph::DirectedGraph dependencies;

	Problem(std::string path) {
		std::ifstream file(path);
		int count = -2;
		graph::Node i = 0, j = 0;
		for (std::string line; std::getline(file, line);) {
			if (read_key(line, "NAME", name)) { continue; }
			if (read_key(line, "COMMENT", comment)) { continue; }

			if (line == "EDGE_WEIGHT_SECTION") {
				count = -1;
				continue;
			}
			if (count == -1) {
				count = std::stoi(line);
				graph = graph::DirectedGraph(count);
				dependencies = graph::DirectedGraph(count);
				continue;
			}

			if (i < count) {
				for (j = 0; j < count; j++) {
					size_t c;
					int n = std::stoi(line, &c, 10);

					if (i != j) {
						if (n == -1) {
							// Dependency
							dependencies.add_edge(j, i);
						}
						else {
							graph::Edge e(i, j);
							graph.add_edge(e);
							weights.emplace(e, n);
						}
					}

					line = line.substr(c);
				}

				i++;
				continue;
			}
		}
		
	}
};