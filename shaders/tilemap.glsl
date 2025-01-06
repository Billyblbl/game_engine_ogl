#define TMX_FLIPPED_HORIZONTALLY 0x80000000
#define TMX_FLIPPED_VERTICALLY   0x40000000
#define TMX_FLIPPED_DIAGONALLY   0x20000000
#define TMX_FLIP_BITS_REMOVAL    0x1FFFFFFF

struct Quad {
	uvec4 layer_sprite;
	vec2 parallax;
	float depth;
};

uniform sampler2D albedo_atlas;
uniform usampler2D tilemap_layers;

layout(std430) restrict readonly buffer Sprites { uvec4 sprites[]; };
layout(std430) restrict readonly buffer Quads { Quad quads[]; };

layout(std140) uniform Scene {
	mat4 view_matrix;
	vec2 parallax_pov;
	float alpha_discard;
};

vec2 sub_uv(vec4 rect, vec2 source_size, vec2 uv) {
	return (uv * (rect.zw - rect.xy) + rect.xy) / source_size;
}

vec4 sample_atlas(sampler2D atlas, vec4 sprite, vec2 uv) {
	return texture(atlas, sub_uv(sprite, textureSize(atlas, 0).xy, uv));
}

uvec4 usample_atlas(usampler2D atlas, vec4 sprite, vec2 uv) {
	return texture(atlas, sub_uv(sprite, textureSize(atlas, 0).xy, uv));
}

smooth pass vec2 layer_uv;
flat pass uvec4 layer;
flat pass uint tilemap_id;

#ifdef VERTEX_SHADER

//*Vertices {
	const vec2 qd_uvs[] = { vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0) };
	in vec2 position;
//*}

void main() {
	Quad quad = quads[gl_VertexID / 4];
	layer_uv = qd_uvs[gl_VertexID % 4];
	layer = quad.layer_sprite;
	gl_Position = view_matrix * vec4(position + parallax_pov * quad.parallax, quad.depth, 1);
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 pixel_color;

void main() {

	uint cell = usample_atlas(tilemap_layers, layer, layer_uv).r;
	//TODO handle flips
	uint gid = cell & TMX_FLIP_BITS_REMOVAL;
	if (gid == 0)
		discard;

	uvec4 tile = sprites[gid];
	vec2 tile_uv = mod(layer_uv * (layer.zw - layer.xy), 1); //* uv coordinates inside the specific tile

	pixel_color = sample_atlas(albedo_atlas, tile, tile_uv);
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
