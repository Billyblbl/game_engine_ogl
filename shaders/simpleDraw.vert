#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) smooth out vec4 outColor;

void main() {
	outColor = color;
	gl_Position = vec4(position, 0.0, 1.0);
}
