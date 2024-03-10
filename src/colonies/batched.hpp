#pragma once
#include <pthread.h>
#include <iostream>

#include "base.hpp"

struct Semaphore {
private:
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int value = 0;

	void check_error(int succ, const char* op, const char* func) {
		if (succ != 0) {
			std::cerr << "[Semaphore] " << op << " returned " << succ << " in " << func << std::endl;
		}
	}
public:
	Semaphore(int value = 0) : value(value) {
		check_error(pthread_mutex_init(&mutex, nullptr), "mutex init", "Semaphore()");
		check_error(pthread_cond_init(&cond, nullptr), "cond init", "Semaphore()");
	}

	Semaphore(const Semaphore&) = delete;

	~Semaphore() {
		check_error(pthread_cond_destroy(&cond), "cond destroy", "~Semaphore()");
		check_error(pthread_mutex_destroy(&mutex), "mutex destroy", "~Semaphore()");
	}

	void reset() {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "reset()");

		value = 0;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "reset()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "reset()");	
	}

	void wait_and_lock(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "wait_and_lock()");
		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "wait_and_lock()");
	
	}

	void unlock() {
		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "unlock()");
	}

	void set(int val) {
		value = val;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "set()");
	}

	void wait_and_reset(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "wait_and_reset()");
		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "wait_and_reset()");

		value = 0;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "wait_and_reset()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "wait_and_reset()");
	}

	void inc_and_wait(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "inc_and_wait()");

		value++;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "inc_and_wait()");

		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "inc_and_wait()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "inc_and_wait()");	
	}
};

template<size_t batch_size = 1>
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
public:
	using AntOptimizer::AntOptimizer;

	std::string name() override { return "batched_" + std::to_string(batch_size); }

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
