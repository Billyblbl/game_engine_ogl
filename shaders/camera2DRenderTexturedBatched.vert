#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(location = 0) smooth out vec2 outUV;

layout(binding = 0) uniform sampler2D texture;

layout(std140, binding = 0) uniform Scene {
	mat4 viewMatrix;
};

layout(std430, binding = 0) buffer Matrices {
	mat4 matrices[];
};

void main() {
	outUV = uv;

	gl_Position = viewMatrix * matrices[gl_InstanceID] * vec4(position, 0.0, 1.0);
}
