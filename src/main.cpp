#include "ant_colony.hpp"
#include "problem.hpp"
#include "workspace.hpp"

#include <chrono>
#include <filesystem>



void print_max_pheromone(const AntOptimizer& optimizer) {
	std::pair<graph::Edge, float> max_pheromone(graph::Edge(-1, -1), std::numeric_limits<float>::min());
	std::pair<graph::Edge, float> min_pheromone(graph::Edge(-1, -1), std::numeric_limits<float>::max());
	for (auto e : optimizer.pheromone_list()) {
		if (e.second > max_pheromone.second) {
			max_pheromone = e;
		}
		
		if (e.second < min_pheromone.second) {
			min_pheromone = e;
		}
		
	}
	std::cout << "  Min pheromone trail: " 
		<< min_pheromone.first.first << " --> "
		<< min_pheromone.first.second << " ("
		<< min_pheromone.second << ")\n";
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

int main(int argc, char* argv[]) {
	std::string problem_path;
	bool interactive = false;
	int rounds = 100;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "-i" || arg == "--interactive") {
			interactive = true;
			continue;
		}

		if (arg == "-r" || arg == "--rounds") {
			i++;
			if (i >= argc) {
				std::cout << "No count given for --rounds parameter" << std::endl;
				return 1;
			}
			try {
				rounds = std::stoi(argv[i]);
			}
			catch(const std::invalid_argument& e) {
				std::cout << "No valid integer: " << argv[i] << std::endl;
			}
			
			continue;
		}

		if (arg == "-h" || arg == "--help") {
			std::cout
			<< "Ant Optimizer\n"
			<< "Usage: ./main [OPTIONS] FILE\n"
			<< "\n"
			<< "Options:\n"
			<< "  -i    --interactive   : Start with GUI and manual control\n"
			<< "  -r N  --rounds N      : Do N optimization steps. Requires [SHIFT] in interactive mode. Default: 100\n"
			<< "  -h    --help          : Show this help page\n"
			<< "\n"
			<< "Interactive mode shortcuts:\n"
			<< "  [L MOUSE BTN]   Drag node plane \n"
			<< "  [MOUSE WHEEL]   Zoom node plane \n"
			<< "  [R]             Reshuffle nodes\n"
			<< "  [SPACE]         Simulate one step of springs. Positions nodes\n"
			<< "  [A]             Simulate 1 ant colony iteration\n"
			<< "  [SHIFT] + [A]   Simulate N ant colony iterations. N specified by -r parameter. Default: 100\n"
			<< "  [ESC]           Quit\n"
			<< "\n";

			return 0;
		}

		problem_path = arg;
	}

	if (!std::filesystem::exists(problem_path)) {
		std::cout << "File '" << problem_path << "' does not exist";
		return 1;
	}

	rounds = std::max(1, rounds);

	Problem problem(problem_path);
	
	std::vector<Ant> ants;
	ants.resize(problem.graph.node_count(), Ant(0));

	int max_dist = 0;
	for (const auto & e : problem.weights) {
		if (e.second == std::numeric_limits<int>::max()) { continue; }
		max_dist = std::max(e.second, max_dist);
	}


	Parameters params;
	params.initial_pheromone = 0.01;
	params.alpha = 1.0;
	params.beta = 0.5;
	params.roh = 0.25;
	params.q = max_dist;
	params.min_pheromone = 0.01;
	params.max_pheromone = 100;
	params.zero_distance = 0.1;

	AntOptimizer colony(
		problem.graph, problem.dependencies, problem.weights,
		ants, params
	);

	if (!interactive) {
		int i = 0;
		auto tp1 = std::chrono::high_resolution_clock::now();
		while (i++ < rounds) {
			colony.optimize();
		}
		auto tp2 = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tp2 - tp1);
		std::cout.precision(4);
		std::cout << "Elapsed: " << elapsed.count() / rounds << "ms average (" << elapsed.count() << "ms total)" << std::endl;

		print_optimizer(colony, problem);

		return 0;
	}

	Workspace workspace(2, problem.graph);

	workspace.edge_color = [&colony](graph::Edge edge) {
		float val = colony.pheromone(edge) / colony.minmax_pheromone().second;
		float c = lerp(0.1, 1.0, val);
		c = easeOutQuad(c);
		return std::make_pair(glm::vec3(0, c, 0), glm::vec3(c, 0, 0));
	};

	workspace.key_callback = [&colony, &workspace, &problem, rounds](int key, int action, int mods) {
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			int iterations = (mods & GLFW_MOD_SHIFT) ? rounds : 1;
			int i = 0;
			auto tp1 = std::chrono::high_resolution_clock::now();
			while (i++ < iterations) {
				colony.optimize();
			}
			auto tp2 = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tp2 - tp1);
			std::cout.precision(4);
			std::cout << "Elapsed: " << elapsed.count() / iterations << "ms average (" << elapsed.count() << "ms total)" << std::endl;

			print_optimizer(colony, problem);
			workspace.prepare_edges();
		}
	};

	workspace.run();

	return 0;
}


