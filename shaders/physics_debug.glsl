
#ifdef VERTEX_SHADER
#define pass out
#endif
#ifdef FRAGMENT_SHADER
#define pass in
#endif

layout(std140, binding = 0) uniform Camera { mat4 view_matrix; };
layout(std140, binding = 1) uniform Object {
	mat4 model_matrix;
	vec4 color;
};

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;

void main() {
	gl_Position = view_matrix * model_matrix * vec4(position, 10, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {
	pixel_color = color;
}

#endif
