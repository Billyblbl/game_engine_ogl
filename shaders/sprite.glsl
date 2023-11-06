
struct InstanceData {
	mat4 matrix;
	uvec4 rect;
	vec4 dimensions; //x,y -> rect dimensions, z -> sprite depth
};

vec2 rect_to_world(uvec4 rect, vec2 v) {
	return v * (rect.zw - rect.xy) + rect.xy;
}

layout(location = 0) smooth pass vec2 texture_uv;
layout(location = 1) flat pass uint instance;

layout(binding = 0) uniform sampler2D atlas;
layout(std430, binding = 0) buffer Entities { InstanceData instances[]; };
layout(std140, binding = 0) uniform Scene {
	mat4 view_matrix;
	uvec4 atlas_dimensions;
	float alpha_discard;
};

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vert_uv;

void main() {
	gl_Position = view_matrix * instances[gl_InstanceID].matrix * vec4(position, instances[gl_InstanceID].dimensions.z, 1.0);
	texture_uv = rect_to_world(instances[gl_InstanceID].rect, vert_uv) / atlas_dimensions.xy;
	instance = gl_InstanceID;
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {
	pixel_color = texture(atlas, texture_uv);
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
