#pragma once

#include "any_ant_colony.hpp"

class SerialAntOptimizer: public AntOptimizer {
public:
	using AntOptimizer::AntOptimizer;

	void optimize() override;
	void optimize(int rounds) override;
};

