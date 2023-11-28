#version 330 core
layout (location = 0) in vec2 p;

out vec2 planePoint;

uniform mat4 proj;
uniform mat4 view;

vec3 UnprojectPoint(vec3 p, mat4 view, mat4 projection) {
	mat4 viewInv = inverse(view);
	mat4 projInv = inverse(projection);
	vec4 unprojectedPoint = viewInv * projInv * vec4(p, 1.0);
	return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
	planePoint = UnprojectPoint(vec3(p, 0.0), view, proj).xy;
	gl_Position = vec4(p, 0.0, 1.0);
}
