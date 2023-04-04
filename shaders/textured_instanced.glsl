
struct InstanceData {
	mat4 matrix;
	uvec2 atlas_index;
};

struct rtf32 {
	vec2 min_corner;
	vec2 max_corner;
};

layout(binding = 0) uniform sampler2DArray atlas;
layout(std430, binding = 0) buffer AtlasMetadata { rtf32 textures_areas[]; };
layout(std430, binding = 1) buffer Entities { InstanceData instances[]; };

layout(std140, binding = 0) uniform Scene { mat4 view_matrix; };

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
// passing to frag
layout(location = 0) smooth out vec2 frag_uv;
layout(location = 1) flat out uint instance;

void main() {
	gl_Position = view_matrix * instances[gl_InstanceID].matrix * vec4(position, 0.0, 1.0);
	rtf32 area = textures_areas[instances[gl_InstanceID].atlas_index.x];

	frag_uv = uv * (area.max_corner - area.min_corner) + area.min_corner;
	instance = gl_InstanceID;
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) smooth in vec2 uv;
layout(location = 1) flat in uint instance;

layout(location = 0) out vec4 pixel_color;

const float alpha_discard = 0.01;

void main() {
	pixel_color = texture(atlas, vec3(uv, instances[instance].atlas_index));
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
