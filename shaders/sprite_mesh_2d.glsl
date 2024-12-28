
struct Entity {
	mat4 transform;
	vec4 color;
	uvec2 state_range;
	// uint padding[2];
};

uniform sampler2D textures[MAX_TEXTURE_IMAGE_UNITS];

layout(std430) restrict readonly buffer Entities { Entity entities[]; };
layout(std430) restrict readonly buffer Sprites { uvec4 sprites[]; };//*xy=min zw=max

layout(std140) uniform Scene {
	mat4 vp_matrix;
	float alpha_discard;
};

vec2 sub_uv(vec4 rect, vec2 source_size, vec2 uv) {
	return (uv * (rect.zw - rect.xy) + rect.xy) / source_size;
}

vec4 sample_atlas(sampler2D atlas, vec4 sprite, vec2 uv) {
	return texture(atlas, sub_uv(sprite, textureSize(atlas, 0).xy, uv));
}

smooth pass vec2 uv;
flat pass uint texture_id;
flat pass uint sprite_id;
smooth pass vec4 color;

//!DEBUG
// smooth pass float d;

#ifdef VERTEX_SHADER

in vec2 position;
in uint albedo_index;
in float depth;

const vec2 uvs[] = { vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0) };

void main() {
	uint entity = gl_BaseInstance + gl_InstanceID;
	uint quad_in_mesh = (gl_VertexID - gl_BaseVertex) / 4;
	sprite_id = entities[entity].state_range.x + quad_in_mesh;
	texture_id = albedo_index;
	color = entities[entity].color;
	uint corner_id = gl_VertexID % 4;
	uv = uvs[corner_id];
	gl_Position = vp_matrix * entities[entity].transform * vec4(position, depth, 1);
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 pixel_color;

void main() {

	pixel_color = color * sample_atlas(textures[texture_id], sprites[sprite_id], uv);
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
