#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader {
protected:
	std::string readFile(const std::string&);
	GLuint compileShader(const std::string&, GLenum);
	GLuint linkProgram(const std::vector<GLuint>&);
public:
	GLuint id;

	Shader(const std::string& vertexPath, const std::string& fragmentPath);
	Shader(const std::string& name) : Shader(name + ".vert", name + ".frag") {}

	void use() const;

	void setBool (const char* name, bool value) const;
	void setInt  (const char* name, int value) const;
	void setFloat(const char* name, float value) const;
	void setMat4 (const char* name, const glm::mat4& value) const;
};
