#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(location = 1) smooth out vec2 outUV;

layout(binding = 1) uniform sampler2D texture;

layout(std140, binding = 0) uniform Scene {
	mat4 viewMatrix;
};

void main() {
	outUV = uv;

	gl_Position = viewMatrix * vec4(position, 0.0, 1.0);
}
