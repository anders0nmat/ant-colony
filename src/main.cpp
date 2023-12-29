#include "ant_colony.hpp"
#include "problem.hpp"
#include "workspace.hpp"

void print_max_pheromone(const AntOptimizer& optimizer) {
	std::pair<graph::Edge, float> max_pheromone(graph::Edge(-1, -1), -1);
	for (auto e : optimizer.pheromone_list()) {
		if (e.second > max_pheromone.second) {
			max_pheromone = e;
		}
	}
	std::cout << "  Max pheromone trail: " 
		<< max_pheromone.first.first << " --> "
		<< max_pheromone.first.second << " ("
		<< max_pheromone.second << ")\n";
}

void print_best_route(const AntOptimizer& optimizer, const Problem& problem) {
	std::cout << "  Best Route (Length = " << optimizer.best_route.length << ") (Bounds: ";
	if (problem.bounds.first != -1 || problem.bounds.second != -1) {
		if (problem.bounds.first != problem.bounds.second) {
			std::cout << "[" << problem.bounds.first << ", " << problem.bounds.second << "]";
		}
		else {
			std::cout << problem.bounds.first;
		}
	}
	else {
		std::cout << "Unknown";
	}
	std::cout << ") :\n    ";

	bool first = true;
	for (const auto node : optimizer.best_route.nodes) {
		if (!first) {
			std::cout << " -- ";
		}
		first = false;
		std::cout << node;
	}
	if (optimizer.best_route.nodes.empty()) {
		std::cout << "No Route";
	}
	std::cout << std::endl;
}

void print_optimizer(const AntOptimizer& optimizer, const Problem& problem) {
	std::cout << "Optimization results (Round "<< optimizer.round << "):\n";
	print_max_pheromone(optimizer);
	print_best_route(optimizer, problem);
}

int main() {
	Problem problem("problems/ESC07.sop");

	std::vector<Ant> ants = {
		Ant(0),
		Ant(0),
		Ant(0),
		Ant(0)
	};

	const float 
		initial_pheromone = 0.01,
		alpha = 1.0,
		beta = 0.5,
		roh = 0.5,
		q = 100.0;

	AntOptimizer colony(
		problem.graph, problem.dependencies, problem.weights,
		ants, initial_pheromone, alpha, beta, roh, q
	);

	Workspace workspace(2, problem.graph);

	workspace.edge_color = [&colony](graph::Edge edge) {
		float val = colony.pheromone(edge) / colony.max_pheromone();
		float c = lerp(0.1, 1.0, val);
		c = easeOutQuad(c);
		return std::make_pair(glm::vec3(0, c, 0), glm::vec3(c, 0, 0));
	};

	workspace.key_callback = [&colony, &workspace, &problem](int key, int action, int mods) {
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			int iterations = (mods & GLFW_MOD_SHIFT) ? 100 : 1;
			while (iterations-- > 0) {
				colony.optimize();
			}

			print_optimizer(colony, problem);
			workspace.prepare_edges();
		}
	};

	workspace.run();
	return 0;
}


