//TODO handle non-character ui quads

struct Text {
	mat4 transform;
	vec4 color;
};

struct Character {
	uint glyph_index;
	uint text_index;
};

struct Atlas {
	uvec2 size;
	uvec2 glyph_range;//*x=min y=max
	uint texture_index;
};

uniform sampler2D textures[MAX_TEXTURE_IMAGE_UNITS];

layout(std430) restrict readonly buffer Fonts{ Atlas fonts[]; };
layout(std430) restrict readonly buffer Characters { Character characters[]; };
layout(std430) restrict readonly buffer Texts { Text texts[]; };
layout(std430) restrict readonly buffer Glyphs { uvec4 glyphs[]; };

layout(std140) uniform Scene {
	mat4 screen_projection;//* based on screen dimensions
	float alpha_discard;
};

vec2 point_in_rect(vec4 rect/*xy=min zw=max*/, vec2 point) {
	return point * (rect.zw - rect.xy) + rect.xy;
}

vec4 sample_atlas(Atlas atlas, uvec4 view, vec2 uv) {
	return texture(textures[atlas.texture_index], point_in_rect(view, uv) / atlas.size);
}

smooth pass vec2 uv;
flat pass uint glyph;
flat pass uint text;
flat pass uint font;

#ifdef VERTEX_SHADER

in vec2 position;

const vec2 uvs[] = { vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0) };

void main() {
	font = gl_DrawID;

	uint ui_id = gl_VertexID / 4;
	glyph = fonts[font].glyph_range.x + characters[ui_id].glyph_index;
	text = characters[ui_id].text_index;

	uint corner_id = gl_VertexID % 4;
	uv = uvs[corner_id];

	gl_Position = screen_projection * texts[text].transform * vec4(position, 0, 1);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

void main() {

	Atlas atlas = fonts[font];
	uvec4 view = glyphs[glyph];
	pixel_color = texts[text].color * sample_atlas(atlas, view, uv).r;
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
