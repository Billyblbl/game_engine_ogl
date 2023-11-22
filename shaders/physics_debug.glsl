
layout(std140, binding = 0) uniform Camera { mat4 vp; };
layout(std140, binding = 1) uniform Object {
	mat4 model;
	vec4 color;
};

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;

void main() {
	gl_Position = vp * model * vec4(position, 10, 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {
	pixel_color = color;
}

#endif
