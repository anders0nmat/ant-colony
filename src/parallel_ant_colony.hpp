#pragma once

#include "any_ant_colony.hpp"

class ParallelAntOptimizer: public AntOptimizer {
private:
	friend void* optimize_threaded(void* __args);
public:
	using AntOptimizer::AntOptimizer;

	void optimize() override;
	void optimize(int rounds) override;
};

