
struct InstanceData {
	mat4 matrix;
	uvec4 view;
};

vec2 atlas_sample(uvec4 view, vec2 uv, vec2 atlas_size) {
	return (uv * (view.zw - view.xy) + view.xy) / atlas_size;
}

layout(location = 0) smooth pass vec2 tilemap_atlas_uv;

layout(binding = 0) uniform sampler2D atlas;
layout(binding = 1) uniform usampler2D tilemap_atlas;

layout(std430, binding = 0) buffer Entities { InstanceData instances[]; };
layout(std430, binding = 1) buffer Tiles { uvec4 tile_buffer[]; };

layout(std140, binding = 0) uniform Scene {
	mat4 view_matrix;
	uvec2 tilemap_atlas_size;
	uvec2 texture_atlas_size;
	float alpha_discard;
};

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vert_uv;

void main() {
	gl_Position = view_matrix * instances[gl_InstanceID].matrix * vec4(position, gl_InstanceID, 1.0);
	uvec4 view = instances[gl_InstanceID].view;
	tilemap_atlas_uv = atlas_sample(view, vert_uv, tilemap_atlas_size);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {
	uint gid = texture(tilemap_atlas, tilemap_atlas_uv).r;
	if (gid == 0)
		discard;
	vec2 tile_uv = mod(tilemap_atlas_uv * tilemap_atlas_size, 1);
	uvec4 tile = tile_buffer[gid];
	vec2 atlas_uv = atlas_sample(tile, tile_uv, texture_atlas_size);
	pixel_color = texture(atlas, atlas_uv);
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
