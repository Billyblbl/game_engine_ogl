#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) smooth out vec4 outColor;

layout(std140, binding = 0) uniform Scene {
	mat4 viewMatrix;
};

void main() {
	outColor = color;
	gl_Position = viewMatrix * vec4(position, 0.0, 1.0);
}
