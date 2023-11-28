#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "shaders.hpp"
#include "undirected_graph.hpp"

class Node {
public:
	static GLuint circle;
	static std::unique_ptr<Shader> node_shader;
	static void init();
	glm::vec2 position;

	Node(glm::vec2 position) : position(position) {
		init();
	}

	void render();
};

class Workspace {
public:
	static GLuint quad;
	static std::unique_ptr<Shader> grid_shader;
	static std::unique_ptr<Shader> edge_shader;
	static void init();

	float grid_spacing;
	UndirectedGraph<std::nullptr_t> graph;
	GLuint edge_mesh;
	GLuint edge_buffer;

	std::vector<Node> nodes;

	Workspace(float grid_spacing);

	void add_node(glm::vec2);
	void remove_node(int);

	void add_edge(int, int);
	void remove_edge(int, int);

	void prepare_edges();

	void simulate_spring(float dampening);
	void simulate_spring_iter(int iterations);

	void render();
};

