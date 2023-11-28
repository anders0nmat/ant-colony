#version 330 core
out vec4 outColor;

in vec2 planePoint;

uniform float spacing;

vec4 grid(vec2 planePos, float spacing) {
	float scale = 1 / spacing;
	vec2 coord = planePos * scale;
	vec2 derivative = fwidth(coord);
	vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
	float line = min(grid.x, grid.y);
	float minX = min(derivative.x, 1);
	float minY = min(derivative.y, 1);
	vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));

	if (abs(planePos.x) < spacing * minX) {
		color.x *= 1.25;
	}

	if (abs(planePos.y) < spacing * minY) {
		color.y *= 1.25;
	}


	return color;
}

void main() {
	outColor = grid(planePoint, spacing);
}