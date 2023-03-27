layout(location = 0) in vec2 position;

layout(std140, binding = 0) uniform Scene {
	mat4 viewMatrix;
};

void main() {
	gl_Position = viewMatrix * vec4(position, 0.0, 1.0);
}
