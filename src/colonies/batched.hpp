#pragma once
#include <pthread.h>
#include <iostream>

#include "base.hpp"
#include "../semaphore.hpp"

class BatchedAntOptimizer: public AntOptimizer {
private:
	struct ThreadArgs {
		Ant* start_ant;
		int ant_count;
		BatchedAntOptimizer& optimizer;
		bool cancelled = false;
	};

	static void* optimize_threaded(void* __args) {
		ThreadArgs* args = static_cast<ThreadArgs*>(__args);

		while (true) {
			args->optimizer.start_line.inc_and_wait(0);
			if (args->cancelled) { return nullptr; }

			const Ant* end_ant = args->start_ant + args->ant_count;
			for (Ant* ant = args->start_ant; ant != end_ant; ant++) {
				for (int i = 0; i < args->optimizer.graph.node_count() - 1; i++) {
					args->optimizer.advance_ant(*ant);
					if (ant->current_node == graph::NO_NODE) { break; }
				}

				if (!args->optimizer.goal_reached(*ant)) { continue; }

				ant->route.length = args->optimizer.route_length(ant->route.nodes);
			}

			args->optimizer.finish_line.inc_and_wait(0);
		}
	}

	Semaphore start_line = Semaphore(0);
	Semaphore finish_line = Semaphore(0);

	std::vector<pthread_t> threads;
	std::vector<ThreadArgs> thread_args;
	std::vector<Ant> ants;
	size_t batch_size = 1;
public:
	using AntOptimizer::AntOptimizer;

	static constexpr const char* _name = "batched";
	std::string name() override { return _name; }

	void init(std::string args) override {
		batch_size = std::stoi(args);
	}

	void optimize() override {
		// Init Ants
		for (int i = 0; i < initial_ants.size(); i++) {
			ants[i] = initial_ants[i];
			ants[i].generator.seed(rand_device());
		}
		const Ant* best_ant = nullptr;

		// Wait for all threads on start line ; Let threads run
		//sem_wait_and_reset(threads.size());
		start_line.wait_and_reset(threads.size());

		// Wait for all threads at finish line ; let them go back to start
		//sem_wait_and_reset(threads.size());
		finish_line.wait_and_reset(threads.size());


		for (Ant & ant : ants) {
			if (ant.route.length == -1) { continue; }

			update_best_route(ant);
			if (best_ant == nullptr || ant.route.length < best_ant->route.length) {
				best_ant = &ant;
			}
		}

		/*
			We don't need to initialize this.
			We call the `[]`-operator, which creates entries with the default initializer
			The default initializer for float returns 0.0, which is what we would initialize this to.
		*/
		std::map<graph::Edge, float> delta_pheromone;

		if (best_ant == nullptr) return;
		for (auto it = std::next(best_ant->route.nodes.begin()); it != best_ant->route.nodes.end(); it++) {
			graph::Edge edge(*std::prev(it), *it);
			delta_pheromone[edge] += pheromone_update(*best_ant, edge);
		}

		for (auto& edge_pair : edge_pheromone) {
			update_edge_pheromone(edge_pair.second, delta_pheromone.count(edge_pair.first) > 0 ? delta_pheromone.at(edge_pair.first) : 0);
		}
		
		round++;
	}

	Profiler optimize(int rounds) override {
		Profiler pf;

		ants = initial_ants;
		size_t first_ant = 0;
		while (first_ant  < initial_ants.size()) {
			int ant_count = std::min(batch_size, initial_ants.size() - first_ant);
			thread_args.emplace_back(ThreadArgs{
				&ants.at(first_ant),
				ant_count,
				*this
			});
			first_ant += ant_count;
		}

		for (auto & args : thread_args) {
			threads.emplace_back();
			int succ = pthread_create(&threads.back(), nullptr, optimize_threaded, static_cast<void*>(&args));
			if (succ != 0) {
				exit(4);
			}
		}
	
		while (rounds-- > 0) {
			pf.start();
			optimize();
			pf.stop();
		}

		// Bring all threads to a stop
		start_line.wait_and_lock(threads.size());

			for (auto & args : thread_args) { args.cancelled = true; }
			start_line.set(0);

		start_line.unlock();

		for (const auto & thread : threads) {
			pthread_join(thread, nullptr);
		}

		return pf;
	}
};
