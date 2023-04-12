
#ifdef VERTEX_SHADER
#define pass out
#endif
#ifdef FRAGMENT_SHADER
#define pass in
#endif

layout(std140, binding = 0) uniform Scene { mat4 view_matrix; };

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;

void main() {
	gl_Position = view_matrix * vec4(position, 0.0, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {
	pixel_color = vec4(1, 0, 0, 1);
}

#endif
