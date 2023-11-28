
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>

#include "workspace.hpp"

std::unique_ptr<Shader> Node::node_shader = nullptr;
GLuint Node::circle = 0;

void Node::init() {
	if (node_shader == nullptr) {
		node_shader = std::make_unique<Shader>("basic");
	}

	if (circle == 0) {
		const float vertices[] = {
			 0,  0,  0,       1, 1, 1,// center
			 0,  1,  0,       1, 0, 0,
			-0.75, 0.75, 0,   1, 1, 0, // top left
			-1, 0, 0,         0, 1, 0,
			-0.75, -0.75, 0,  0, 1, 1,
			0, -1, 0,         0, 0, 1,
			0.75, -0.75, 0,   1, 0, 1,
			1, 0, 0,          1, 0, 0.5,
			0.75, 0.75, 0,     1, 0.5, 0.25,
			 0,  1,  0,       1, 0, 0,
		};

		glGenVertexArrays(1, &circle);
		glBindVertexArray(circle);
		
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0); // Clear VAO
	}
}

void Node::render() {
	node_shader->use();
	node_shader->setMat4("model", glm::scale(glm::translate(glm::mat4(), glm::vec3(position.x, position.y, 0)), glm::vec3(0.2)));

	glBindVertexArray(circle);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
}

std::unique_ptr<Shader> Workspace::grid_shader = nullptr;
std::unique_ptr<Shader> Workspace::edge_shader = nullptr;
GLuint Workspace::quad = 0;

void Workspace::init() {
	if (grid_shader == nullptr) {
		grid_shader = std::make_unique<Shader>("grid");
	}

	if (edge_shader == nullptr) {
		edge_shader = std::make_unique<Shader>("edges");
	}

	if (quad == 0) {
		const float vertices[] = {
			 1, -1, // bottom right
			-1, -1, // bottom left
			 1,  1, // top right

			 1,  1, // top right
			-1,  1, // top left
			-1, -1
		};

		glGenVertexArrays(1, &quad);
		glBindVertexArray(quad);
		
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0); // Clear VAO
	}
}

Workspace::Workspace(float grid_spacing) : grid_spacing(grid_spacing) {
	init();

	glGenVertexArrays(1, &edge_mesh);
	glBindVertexArray(edge_mesh);
	
	glGenBuffers(1, &edge_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Workspace::render() {
	grid_shader->use();
	grid_shader->setFloat("spacing", grid_spacing);
	glBindVertexArray(quad);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	edge_shader->use();
	edge_shader->setMat4("model", glm::mat4());
	glBindVertexArray(edge_mesh);
	glDrawArrays(GL_LINES, 0, graph.edge_count() * 2);

	std::for_each(nodes.begin(), nodes.end(), [](Node& n){ n.render(); });
}

void Workspace::add_node(glm::vec2 pos) {
	graph.add_node();
	nodes.push_back(Node(pos));
}

void Workspace::remove_node(int node) {
	if (node >= nodes.size()) { return; }
	graph.remove_node(node);
	nodes.erase(nodes.begin() + node);
}

void Workspace::add_edge(int a, int b) {
	graph.add_edge(a, b);

	glBindVertexArray(edge_mesh);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2 * 2 * graph.edge_count(), NULL, GL_DYNAMIC_DRAW);
}

void Workspace::remove_edge(int a, int b) {
	graph.remove_edge(a, b);

	glBindVertexArray(edge_mesh);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2 * 2 * graph.edge_count(), NULL, GL_DYNAMIC_DRAW);
}

void Workspace::prepare_edges() {
	std::vector<glm::vec3> buffer;

	for (const auto & e : graph.edges) {
		buffer.push_back(glm::vec3(nodes[e->nodes.first].position, 0.0));
		buffer.push_back(glm::vec3(1, 0, 0));
		buffer.push_back(glm::vec3(nodes[e->nodes.second].position, 0.0));
		buffer.push_back(glm::vec3(0, 1, 0));
	}

	glBindVertexArray(edge_mesh);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * buffer.size(), (void*)buffer.data());
}

void Workspace::simulate_spring(float dampening) {
	const float SPRING_REST_LENGTH = 2;
	const float TIME_STEP = dampening;
	std::vector<glm::vec2> force(nodes.size());

	for (const auto & e : graph.edges) {
		glm::vec2 pos1 = nodes[e->nodes.first].position;
		glm::vec2 pos2 = nodes[e->nodes.second].position;

		glm::vec2 d1 = pos2 - pos1;
		if (glm::length(d1) < SPRING_REST_LENGTH) {
			d1 = -d1;
		}

		d1 *= dampening;

		force[e->nodes.first] += d1;
		force[e->nodes.second] -= d1;						
	}

	for (int i = 0; i < graph.node_count(); i++) {
		for (int ii = i + 1; ii < graph.node_count(); ii++) {
			glm::vec2 pos1 = nodes[i].position;
			glm::vec2 pos2 = nodes[ii].position;
			float d = glm::distance(pos1, pos2);
			if (d < 2) {
				glm::vec2 dist = pos2 - pos1;
				if (d == 0) { 
					d = 0.1;
					dist = glm::vec2(1, 1);
				}
				force[i] -= dist * (0.1f / d);
				force[ii] += dist * (0.1f / d);
			}			
		}
	}

	for (int i = 0; i < nodes.size(); i++) {
		nodes[i].position += force[i];
	}
}

void Workspace::simulate_spring_iter(int iterations) {
	for (int i = 0; i < iterations; i++) {
		double damp = 1 - sin((i / iterations) * 3.1415 / 2);
		damp *= 0.1;
		simulate_spring(damp);
	}
}

