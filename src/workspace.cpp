
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <random>
#include <chrono>

#include "workspace.hpp"
#include "animated_value.hpp"
#include "camera.hpp"

namespace {
	/* Helper Functions */

	glm::vec2 randomCircle(float radius = 1) {
		constexpr double pi = 3.14159265358979323846;
		std::random_device rd;
		std::default_random_engine generator(rd());
		std::uniform_real_distribution distribution(0.0, 2 * pi);
		double angleRad = distribution(generator);
		double x, y;
		sincos(angleRad, &y, &x);
		return glm::vec2(x, y) * radius;
	}

}

namespace {
	bool isInitialized = false;

	GLFWwindow* window = nullptr;
	Camera cam(glm::vec3(0, 0, 2), glm::vec2(8, 6));

	GLuint quad;
	GLuint circle;
	std::unique_ptr<Shader> node_shader;
	std::unique_ptr<Shader> grid_shader;
	std::unique_ptr<Shader> edge_shader;

	AnimatedValue<glm::vec2, float> camScale(glm::vec2(8, 6), easeOutQuad<float>);
}

namespace {
	/* Callbacks */
	Workspace* current = nullptr;

	bool drag = false;
	glm::vec2 lastMousePos;

	void processInput(GLFWwindow* wnd) {
		if (glfwGetKey(wnd, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(wnd, true);
		}

		cam.size = camScale.current();
		//cam.pos = camPos.current();
	}

	void framebuffer_size_callback(GLFWwindow* wnd, int width, int height) {
		glViewport(0, 0, width, height);
		camScale.end_value.y = camScale.end_value.x * ((float)height / (float)width);
	}

	void scroll_callback(GLFWwindow* wnd, double xoff, double yoff) {
		const float SENSITIVITY = 0.1;
		const float zoom_factor = 1 - yoff * SENSITIVITY;

		camScale.ease_to(camScale.end_value * zoom_factor, 0.2);
	}

	void click_callback(GLFWwindow* wnd, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			drag = action == GLFW_PRESS;
			if (drag) {
				double x, y;
				glfwGetCursorPos(wnd, &x, &y);
				lastMousePos = glm::vec2(x, y);
			}
		}
	}

	void cursor_pos_callback(GLFWwindow* wnd, double xpos, double ypos) {
		if (drag) {
			glm::vec2 pos(xpos, ypos);

			glm::vec2 diff = pos - lastMousePos;
			diff = glm::vec2(-diff.x, diff.y);
			
			glm::ivec2 view;
			glfwGetWindowSize(wnd, &view.x, &view.y);
			diff *= cam.size / glm::vec2(view);

			cam.move(diff);
			//camPos.ease_to(camPos.end_value + glm::vec3(diff, 0), 0.05);
			lastMousePos = pos;
		}
	}

	void keyboard_callback(GLFWwindow* wnd, int key, int scancode, int action, int mods) {
		if (current != nullptr) {
			current->_keyboard_callback(wnd, key, action, mods);
		}
	}
}

NodePositions::NodePositions(const graph::DirectedGraph& graph, std::function<double(int)> dampening)
: graph(graph), positions(graph.node_count()), damp_func(dampening) {
	for (auto & pos : positions) {
		pos = randomCircle();
	}
}

void NodePositions::reset() {
	iterations = 0;
	for (auto& e : positions) {
		e = randomCircle();
	}
}

void NodePositions::simulate_spring() {
	const float dampening = damp_func(iterations);
	const float SPRING_REST_LENGTH = 4;
	std::vector<glm::vec2> force(graph.node_count());

	for (const auto & e : graph.edges) {
		glm::vec2 pos1 = positions[e.first];
		glm::vec2 pos2 = positions[e.second];

		glm::vec2 d1 = pos2 - pos1;
		if (glm::length(d1) < SPRING_REST_LENGTH) {
			d1 = -d1;
		}

		d1 *= dampening;

		force[e.first] += d1;
		force[e.second] -= d1;						
	}

	for (int i = 0; i < graph.node_count(); i++) {
		for (int ii = i + 1; ii < graph.node_count(); ii++) {
			glm::vec2 pos1 = positions[i];
			glm::vec2 pos2 = positions[ii];
			float d = glm::distance(pos1, pos2);
			if (d < SPRING_REST_LENGTH) {
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

	for (int i = 0; i < positions.size(); i++) {
		positions[i] += force[i];
	}

	iterations++;
}

void NodePositions::simulate_spring_iter(int iterations) {
	for (int i = 0; i < iterations; i++) {
		simulate_spring();
	}
}

bool Workspace::initBoard() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(800, 600, "Ant Optimization", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to init GLAD" << std::endl;
		return false;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glViewport(0, 0, 800, 600);

	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, click_callback);
	glfwSetKeyCallback(window, keyboard_callback);

	glLineWidth(2);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return true;
}

void Workspace::init() {
	if (isInitialized) { return; }
	if (!initBoard()) { exit(-1); }
	current = this;

	grid_shader = std::make_unique<Shader>("grid");
	edge_shader = std::make_unique<Shader>("edges");
	node_shader = std::make_unique<Shader>("basic");

	/* Quad */ {
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

	/* Circle */ {
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

void Workspace::_keyboard_callback(GLFWwindow* wnd, int key, int action, int mods) {
	if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		positions.simulate_spring();
		prepare_edges();
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		positions.reset();
		positions.simulate_spring_iter(1000);
		prepare_edges();
	}

	if (key_callback) {
		key_callback(key, action, mods);
	}
}


Workspace::Workspace(
	float grid_spacing,
	const graph::DirectedGraph& graph)
	: grid_spacing(grid_spacing), graph(graph), positions(graph) {
	init();

	positions.simulate_spring_iter(1000);

	glGenVertexArrays(1, &edge_mesh);
	glBindVertexArray(edge_mesh);
	
	glGenBuffers(1, &edge_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	prepare_edges();

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
	glDrawArrays(GL_LINES, 0, graph.edge_count() * 4);

	node_shader->use();
	glBindVertexArray(circle);
	for (int node = 0; node < graph.node_count(); node++) {
		node_shader->setMat4("model", glm::scale(glm::translate(glm::mat4(), glm::vec3(positions.positions.at(node), 0)), glm::vec3(0.2)));

		glDrawArrays(GL_TRIANGLE_FAN, 0, 10);	
	}
}

void Workspace::prepare_edges() {
	std::vector<glm::vec3> buffer;

	for (const auto & e : graph.edges) {
		std::pair<glm::vec3, glm::vec3> colors(glm::vec3(0.7), glm::vec3(0.4));
		if (edge_color) {
			colors = edge_color(e);
		}
		auto e1pos = positions.positions.at(e.first);
		auto e2pos = positions.positions.at(e.second);
		glm::vec2 joint = e2pos - e1pos; std::swap(joint.x, joint.y); joint.y *= -1;
		joint *= 0.02;
		glm::vec2 halfway = (e1pos + e2pos) * 0.5f;
		glm::vec2 joint_halfway = halfway + joint;
		glm::vec3 halfway_color = (colors.first + colors.second) * 0.5f;

		buffer.push_back(glm::vec3(e1pos, 0.0));
		buffer.push_back(colors.first);

		buffer.push_back(glm::vec3(joint_halfway, 0.0));
		buffer.push_back(halfway_color);

		buffer.push_back(glm::vec3(joint_halfway, 0.0));
		buffer.push_back(halfway_color);

		buffer.push_back(glm::vec3(e2pos, 0.0));
		buffer.push_back(colors.second);
	}

	glBindVertexArray(edge_mesh);
	glBindBuffer(GL_ARRAY_BUFFER, edge_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * buffer.size(), (void*)buffer.data(), GL_DYNAMIC_DRAW);
}

void Workspace::run() {
	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		grid_shader->use();
		grid_shader->setMat4("proj", cam.getProjectionMatrix());
		grid_shader->setMat4("view", cam.getViewMatrix());
		edge_shader->use();
		edge_shader->setMat4("projection", cam.getProjectionMatrix());
		edge_shader->setMat4("view", cam.getViewMatrix());
		node_shader->use();
		node_shader->setMat4("projection", cam.getProjectionMatrix());
		node_shader->setMat4("view", cam.getViewMatrix());
		render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();	
}

