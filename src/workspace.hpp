#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "shaders.hpp"
#include "graph.hpp"

struct NodePositions {
	const graph::DirectedGraph& graph;
	std::vector<glm::vec2> positions;
	int iterations = 0;
	std::function<double(int)> damp_func;

	NodePositions(const graph::DirectedGraph& graph, std::function<double(int)> dampening = [](int i){ return 0.1 * (1 - sin(i / 1000) * 3.1415 / 2); });
	
	void reset();
	void simulate_spring();
	void simulate_spring_iter(int iterations);
};

class Workspace {
private:
	using EdgeColorFunc = std::function<std::pair<glm::vec3, glm::vec3>(graph::Edge)>;
	using KeyFunc = std::function<void(int, int, int)>;

	void init();
	bool initBoard();


	float grid_spacing;
	const graph::DirectedGraph& graph;
	NodePositions positions;
	
	GLuint edge_mesh;
	GLuint edge_buffer;
public:
	void _keyboard_callback(GLFWwindow* wnd, int key, int action, int mods);
	KeyFunc key_callback;
	EdgeColorFunc edge_color;

	Workspace(
		float grid_spacing,
		const graph::DirectedGraph& graph);

	void prepare_edges();
	void render();

	void run();
};

template<typename T>
T easeOutQuad(T x) {
	return 1 - (1 - x) * (1 - x);
}

template<typename T>
T lerp(T from, T to, float a) {
	return (1 - a) * from + (a) * to;
}

