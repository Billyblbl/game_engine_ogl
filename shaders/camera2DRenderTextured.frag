#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 1) smooth in vec2 uv;

layout(binding = 1) uniform sampler2D texture;

// layout(std140, binding = 0) uniform Scene {
// 	mat4 viewMatrix;
// }

void main() {
	outColor = texture2D(texture, uv);
	// outColor = vec4(1, 0.0, 0.0, 1.0);
}
