#pragma once

#include "base.hpp"

class SerialAntOptimizer: public AntOptimizer {
public:
	using AntOptimizer::AntOptimizer;

	void optimize() override;
	void optimize(int rounds) override;
};

