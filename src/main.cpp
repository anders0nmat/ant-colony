
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

Camera cam(glm::vec3(0, 0, 2), glm::vec2(8, 6));

constexpr double pi = 3.14159265358979323846;

bool drag = false;
glm::vec2 lastMousePos;
Workspace* workspace = nullptr;
int spring_iteration = 1;

AnimatedValue<glm::vec2, float> camScale(glm::vec2(8, 6), [](double x) { return 1 - (1 - x) * (1 - x); });

glm::vec2 randomCircle(float radius = 1) {
	std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_real_distribution distribution(0.0, 2 * pi);
	double angleRad = distribution(generator);
	double x, y;
	sincos(angleRad, &y, &x);
	return glm::vec2(x, y) * radius;
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
		workspace->simulate_spring(1.0f / (float)spring_iteration);
		workspace->prepare_edges();
		spring_iteration++;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		for (auto& e : workspace->nodes) {
			e.position = randomCircle();
		}

		workspace->simulate_spring_iter(1000);
		workspace->prepare_edges();
	}
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW Window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to init GLAD" << std::endl;
		return -1;
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

	Workspace ws(2);
	workspace = &ws;
	
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_node(randomCircle());
	ws.add_edge(0, 1);
	ws.add_edge(0, 2);
	ws.add_edge(1, 2);
	ws.add_edge(1, 3);
	ws.add_edge(3, 4);
	ws.add_edge(4, 5);
	ws.add_edge(2, 5);
	ws.add_edge(5, 6);
	ws.add_edge(6, 7);
	ws.add_edge(7, 8);
	ws.add_edge(7, 9);
	ws.add_edge(8, 9);
	ws.simulate_spring_iter(1000);
	ws.prepare_edges();
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cout << "OpenGL threw error: " << err << std::endl;
	}


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
	
		Node::node_shader->use();
		Node::node_shader->setMat4("projection", cam.getProjectionMatrix());
		Node::node_shader->setMat4("view", cam.getViewMatrix());
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


