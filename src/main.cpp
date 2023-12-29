
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <chrono>

#include "shaders.hpp"
#include "camera.hpp"
#include "workspace.hpp"
#include "animatedValue.hpp"
#include "problem.hpp"
#include "ant_colony.hpp"

Camera cam(glm::vec3(0, 0, 2), glm::vec2(8, 6));

bool drag = false;
glm::vec2 lastMousePos;
//Workspace* workspace = nullptr;
int spring_iteration = 1;
AntOptimizer* optimizer;

AnimatedValue<glm::vec2, float> camScale(glm::vec2(8, 6), [](double x) { return 1 - (1 - x) * (1 - x); });

glm::vec2 randomCircle(float radius = 1) {
	constexpr double pi = 3.14159265358979323846;
	std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_real_distribution distribution(0.0, 2 * pi);
	double angleRad = distribution(generator);
	double x, y;
	sincos(angleRad, &y, &x);
	return glm::vec2(x, y) * radius;
}

struct NodePositions {
	const graph::DirectedGraph& graph;
	std::vector<glm::vec2> positions;

	NodePositions(const graph::DirectedGraph& graph)
	: graph(graph), positions(graph.node_count()) {
		for (auto & pos : positions) {
			pos = randomCircle();
		}
	}

	void simulate_spring(float dampening) {
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
	}

	void simulate_spring_iter(int iterations) {
		for (int i = 0; i < iterations; i++) {
			double damp = 1 - sin((i / iterations) * 3.1415 / 2);
			damp *= 0.1;
			simulate_spring(damp);
		}
	}
};

NodePositions* node_positions;

void print_max_pheromone() {
	std::pair<graph::Edge, float> max_pheromone(graph::Edge(-1, -1), -1);
	for (auto e : optimizer->edge_pheromone) {
		if (e.second > max_pheromone.second) {
			max_pheromone = e;
		}
	}
	std::cout << "  Max pheromone trail: " 
		<< max_pheromone.first.first << " --> "
		<< max_pheromone.first.second << " ("
		<< max_pheromone.second << ")\n";
}

void print_best_route() {
	std::cout << "  Best Route (Length = " << optimizer->best_route.length << "):\n    ";
	bool first = true;
	for (const auto node : optimizer->best_route.nodes) {
		if (!first) {
			std::cout << " -- ";
		}
		first = false;
		std::cout << node;
	}
	if (optimizer->best_route.nodes.empty()) {
		std::cout << "No Route";
	}
	std::cout << std::endl;
}

void print_optimizer() {
	std::cout << "Optimization results (Round "<< optimizer->round << "):\n";
	print_max_pheromone();
	print_best_route();
}

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
	//cam.zoom(zoom_factor);
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
	if (key == GLFW_KEY_SPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		node_positions->simulate_spring(1.0f / (float)spring_iteration);
		workspace->prepare_edges();
		spring_iteration++;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		for (auto& e : node_positions->positions) {
			e = randomCircle();
		}

		node_positions->simulate_spring_iter(1000);
		workspace->prepare_edges();
	}

	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		int iterations = 1;
		if (mods & GLFW_MOD_SHIFT) {
			iterations = 100;
		}
		for (;iterations > 0;iterations--) {
			optimizer->optimize();
		}
		print_optimizer();
		workspace->prepare_edges();
	}
}

template<typename T>
T lerp(T from, T to, float a) {
	return (1 - a) * from + (a) * to;
}

float easeCircle(float x) {
	return 1 - (1 - x) * (1 - x);
}

bool initBoard() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);
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

void key_callback(int key, int action, int mods) {
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		int iterations = 1;
		if (mods & GLFW_MOD_SHIFT) {
			iterations = 100;
		}
		for (;iterations > 0;iterations--) {
			optimizer->optimize();
		}
		print_optimizer();
		workspace->prepare_edges();
	}
}

int main() {
	if (!initBoard()) {
		return -1;
	}

	Problem problem("problems/ESC07.sop");

	std::vector<Ant> ants = {
		Ant(0)
	};

	AntOptimizer colony(
		problem.graph,
		problem.dependencies,
		problem.weights,
		ants,
		0.01,   // initial pheromone
		1.0,    // alpha
		0.5,    // beta
		0.5,    // roh
		100.0   // Q
	);
	optimizer = &colony;

	Workspace workspace(2, problem.graph);
	workspace.edge_color = [&colony](graph::Edge edge) {
		float val = colony.edge_pheromone[edge] / colony.max_pheromone();
		float c = lerp(0.1, 1.0, val);
		c = easeOutQuad(c);
		return std::make_pair(glm::vec3(0, c, 0), glm::vec3(c, 0, 0));
	};

	workspace.key_callback = [&colony, &workspace](int key, int action, int mods) {
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			int iterations = 1;
			if (mods & GLFW_MOD_SHIFT) {
				iterations = 100;
			}
			for (;iterations > 0;iterations--) {
				optimizer->optimize();
			}
			print_optimizer();
			workspace.prepare_edges();
		}
	};

	//workspace = &ws;
	workspace.prepare_edges();
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cout << "OpenGL threw error: " << err << std::endl;
	}

	workspace.run();

	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		Workspace::grid_shader->use();
		Workspace::grid_shader->setMat4("proj", cam.getProjectionMatrix());
		Workspace::grid_shader->setMat4("view", cam.getViewMatrix());
		Workspace::edge_shader->use();
		Workspace::edge_shader->setMat4("projection", cam.getProjectionMatrix());
		Workspace::edge_shader->setMat4("view", cam.getViewMatrix());
		Workspace::node_shader->use();
		Workspace::node_shader->setMat4("projection", cam.getProjectionMatrix());
		Workspace::node_shader->setMat4("view", cam.getViewMatrix());
		ws.render();

		// updateNodes(); // Business logic
		
		// renderUI();
		//   renderText();
		//   renderButtons();
		// renderNodes();


		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	return 0;
}


