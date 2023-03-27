layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform Scene {
	mat4 viewMatrix;
};

// layout(location = 0) uniform vec4 color;

void main() {
	// outColor = color;
	outColor = vec4(1, 0.0, 0.0, 1.0);
}
