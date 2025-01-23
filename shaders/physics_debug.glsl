
struct Instance {
	mat3 transform;
	vec4 color;
};

layout(std140) uniform Camera { mat4 vp; };
layout(std430) readonly restrict buffer Instances { Instance instances[]; };

flat pass vec4 color;

#ifdef VERTEX_SHADER

in vec2 position;

void main() {
	color = instances[gl_BaseInstance + gl_InstanceID].color;
	gl_Position = vp * vec4(instances[gl_BaseInstance + gl_InstanceID].transform * vec3(position, 1), 1.0);
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 pixel_color;

void main() {
	pixel_color = color;
}

#endif
