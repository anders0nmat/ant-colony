#include "shaders.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>

std::string Shader::readFile(const std::string& path) {
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		file.open(path);
		std::stringstream stream;

		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}
	catch (std::ifstream::failure e) {
		std::cout << "ERROR::SHADER::FILE_READ: " << path << std::endl;
	}
	return "";
}

GLuint Shader::compileShader(const std::string& source, GLenum type) {
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::string type_name = "UNKNOWN";
		switch (type) {
			case GL_VERTEX_SHADER: type_name = "VERTEX"; break;
			case GL_FRAGMENT_SHADER: type_name = "FRAGMENT"; break;
			case GL_GEOMETRY_SHADER: type_name = "GEOMETRY"; break;
		}
		std::cout << "ERROR::SHADER::" << type_name << "_COMPILE\n" << infoLog << std::endl;
	}

	return shader;
}

GLuint Shader::linkProgram(const std::vector<GLuint>& shaders) {
	GLuint program = glCreateProgram();
	
	std::for_each(shaders.cbegin(), shaders.cend(), std::bind(glAttachShader, program, std::placeholders::_1));

	glLinkProgram(program);

	int success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::LINKING\n" << infoLog << std::endl;
	}
	
	return program;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
	std::string 
		vertexCode = readFile(vertexPath),
		fragmentCode = readFile(fragmentPath);

	std::vector<GLuint> programs = {
		compileShader(vertexCode, GL_VERTEX_SHADER),
		compileShader(fragmentCode, GL_FRAGMENT_SHADER),
	};

	id = linkProgram(programs);

	std::for_each(programs.cbegin(), programs.cend(), glDeleteShader);
}

void Shader::use() const {
	glUseProgram(id);
}

void Shader::setBool(const char* name, bool value) const {
	glUniform1i(glGetUniformLocation(id, name), value ? 1 : 0);
}

void Shader::setInt(const char* name, int value) const {
	glUniform1i(glGetUniformLocation(id, name), value);
}

void Shader::setFloat(const char* name, float value) const {
	glUniform1f(glGetUniformLocation(id, name), value);
}

void Shader::setMat4(const char* name, const glm::mat4& value) const {
	glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(value));
}


