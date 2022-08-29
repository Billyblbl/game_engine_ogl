#version 450 core

layout(location = 0) smooth in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texture;

// layout(std140, binding = 0) uniform Scene {
// 	mat4 viewMatrix;
// }

void main() {
	outColor = texture2D(texture, uv);
	// outColor = vec4(1, 0.0, 0.0, 1.0);
}
