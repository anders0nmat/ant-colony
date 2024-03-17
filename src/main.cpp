#include "colonies/serial.hpp"
#include "colonies/parallel.hpp"
#include "colonies/batched.hpp"
#include "colonies/threaded.hpp"

#include "problem.hpp"
#include "workspace.hpp"

#include <chrono>
#include <filesystem>
#include <memory>

struct CliParams {
	std::string colony_identifier = "serial";
	bool interactive = false;
	bool profiler = false;
	bool verbose = false;
	bool list = false;
	int rounds = 100;
	std::filesystem::path problem_path;

	CliParams(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			std::string arg = argv[i];

			if (arg == "-i" || arg == "--interactive") {
				interactive = true;
				continue;
			}
			
			if (arg == "-v" || arg == "--verbose") {
				verbose = true;
				continue;
			}

			if (arg == "-t" || arg == "--type") {
				i++;
				if (i >= argc) {
					std::cout << "No name given for " << arg << " parameter" << std::endl;
					exit(1);
				}
				
				colony_identifier = argv[i];

				continue;
			}

			if (arg == "-p" || arg == "--profiler") {
				profiler = true;
				continue;
			}

			if (arg == "-l" || arg == "--list") {
				list = true;
				continue;
			}

			if (arg == "-r" || arg == "--rounds") {
				i++;
				if (i >= argc) {
					std::cout << "No count given for " << arg << " parameter" << std::endl;
					exit(1);
				}
				try {
					rounds = std::stoi(argv[i]);
				}
				catch(const std::invalid_argument& e) {
					std::cout << "No valid integer: " << argv[i] << std::endl;
					exit(1);
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
				<< "  -v    --verbose       : Output information about colony. Enabled by default in interactive mode\n"
				<< "  -t    --type          : Specify other colony implementation. Default: serial \n"
				<< "  -l    --list          : List types of colony implementations \n"
				<< "  -p    --profiler      : Append results to file. Location: <problem_folder>/profiler/<problem_name>_<implementation_name>.txt\n"
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

				exit(0);
			}

			problem_path = arg;
		}

		rounds = std::max(1, rounds);
	}
};

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

Profiler run_colony(AntOptimizer& optimizer, int rounds = 1) {
	auto tp1 = std::chrono::high_resolution_clock::now();

	Profiler pf = optimizer.optimize(rounds);

	auto tp2 = std::chrono::high_resolution_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tp2 - tp1).count();
	double avg = elapsed / rounds;

	std::cout.precision(4);
	std::cout
		<< "[" << optimizer.name() << ":" << optimizer.init_args << "] "
		<< avg
		<< "ms average ("
		<< elapsed
		<< "ms total)"
		<< std::endl;

	return pf;
}

struct AbstractColonyFactory {
	virtual std::string name() const = 0;
	virtual std::unique_ptr<AntOptimizer> make(const Problem& problem, const std::vector<Ant>& ants, Parameters params, std::string args) = 0;
	virtual ~AbstractColonyFactory() = default;
};

template<typename Ty>
struct ColonyFactory: AbstractColonyFactory {
	std::string name() const override { return Ty::_name; }

	std::unique_ptr<AntOptimizer> make(const Problem& problem, const std::vector<Ant>& ants, Parameters params, std::string args) override {
		return std::make_unique<Ty>(problem.graph, problem.dependencies, problem.weights, ants, params, args);
	}
};

std::vector<std::unique_ptr<AbstractColonyFactory>> colonies;
void init_colonies() {
	#define add(CLASS) do { colonies.emplace_back(std::make_unique<ColonyFactory<CLASS>>()); } while(0)

	add(SerialAntOptimizer);
	add(ParallelAntOptimizer);
	add(BatchedAntOptimizer);
	add(ThreadedAntOptimizer);

	#undef add
}

std::unique_ptr<AntOptimizer> makeColony(std::string colony_constructor, const Problem& problem, std::vector<Ant>& ants, Parameters params) {
	auto sep = colony_constructor.find_first_of(":");
	std::string identifier = colony_constructor.substr(0, sep);
	std::string args = (sep != std::string::npos ? colony_constructor.substr(sep + 1) : "");

	auto it = std::find_if(colonies.begin(), colonies.end(), [&](const std::unique_ptr<AbstractColonyFactory>& e){ return e->name() == identifier; });

	if (it == colonies.end()) {
		std::cout << "Unknown colony: " << identifier << std::endl;
		exit(1);
	}

	return (*it)->make(problem, ants, params, args);
}

std::string print_duration(Profiler::Duration d) {
	return std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(d).count()) + " µs";
}

std::string print_now() {
	auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char now_str[std::size("2000-01-01T16:00:00")];
	std::strftime(now_str, std::size(now_str), "%FT%T", std::gmtime(&current_time));
	return std::string(now_str);
}

std::string print_params(Parameters params) {
	std::string result = "";
	result += "alpha: " + std::to_string(params.alpha) + ", ";
	result += "beta: " + std::to_string(params.beta) + ", ";
	result += "roh: " + std::to_string(params.roh) + ", ";
	result += "q: " + std::to_string(params.q) + ", ";
	result += "initial_pheromone: " + std::to_string(params.initial_pheromone) + ", ";
	result += "min_pheromone: " + std::to_string(params.min_pheromone) + ", ";
	result += "max_pheromone: " + std::to_string(params.max_pheromone) + ", ";
	result += "zero_distance: " + std::to_string(params.zero_distance);

	return result;
}

void append_profiler(std::filesystem::path path, const Profiler& pf, AntOptimizer* colony) {
	std::ofstream file(path, std::ios::app);
	auto mm = pf.min_max();
	file 
		<< "### " << print_now() << " ###\n"
		<< "solution=" << colony->best_route.length << "\n"
		<< "rounds=" << pf.durations.size() << "\n"
		<< "total=" << print_duration(pf.total()) << "\n"
		<< "avg=" << print_duration(pf.avg()) << "\n"
		<< "min=" << print_duration(mm.first) << "\n"
		<< "max=" << print_duration(mm.second) << "\n"
		<< "params=" << print_params(colony->params) << "\n"
		<< "args=" << colony->init_args << "\n"
		<< "\n";
}

int main(int argc, char* argv[]) {
	init_colonies();
	CliParams cli(argc, argv);

	if (cli.list) {
		for (const auto& e : colonies) {
			std::cout << e->name() << '\n';
		}
		return 0;
	}

	if (!std::filesystem::exists(cli.problem_path)) {
		std::cout << "File '" << cli.problem_path << "' does not exist";
		exit(1);
	}
	Problem problem(cli.problem_path);
	
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

	if (!cli.interactive) {
		std::vector<std::string> colony_options = {
			"serial", "parallel", "batched:1", "batched:15", "threaded:auto", "threaded:4"
		};

		if (cli.colony_identifier != "all") {
			colony_options.clear();
			colony_options.push_back(cli.colony_identifier);
		}

		for (const auto & option : colony_options) {
			std::unique_ptr<AntOptimizer> colony = makeColony(option, problem, ants, params);
			Profiler pf = run_colony(*colony, cli.rounds);

			if (cli.profiler) {
				auto profile = cli.problem_path.parent_path() / "profiler" / (cli.problem_path.stem().string() + "_" + colony->name() + ".txt");
				std::filesystem::create_directory(profile.parent_path());
				append_profiler(profile, pf, colony.get());
			}

			if (cli.verbose) {
				print_optimizer(*colony, problem);
			}	
		}

		return 0;
	}

	std::unique_ptr<AntOptimizer> colony = makeColony(cli.colony_identifier, problem, ants, params);
	Workspace workspace(2, problem.graph);

	workspace.edge_color = [&colony](graph::Edge edge) {
		float val = colony->pheromone(edge) / colony->minmax_pheromone().second;
		float c = lerp(0.1, 1.0, val);
		c = easeOutQuad(c);
		return std::make_pair(glm::vec3(0, c, 0), glm::vec3(c, 0, 0));
	};

	workspace.key_callback = [&colony, &workspace, &problem, &cli](int key, int action, int mods) {
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			int iterations = (mods & GLFW_MOD_SHIFT) ? cli.rounds : 1;
			run_colony(*colony, iterations);

			print_optimizer(*colony, problem);
			workspace.prepare_edges();
		}
	};

	workspace.run();

	return 0;
}


