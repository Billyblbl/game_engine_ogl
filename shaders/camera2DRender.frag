#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 0) smooth in vec4 color;

// layout(std140, binding = 0) uniform Scene {
// 	mat4 viewMatrix;
// }

void main() {
	outColor = color;
	// outColor = vec4(1, 0.0, 0.0, 1.0);
}
