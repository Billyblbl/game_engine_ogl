vec2 point_in_rect(vec4 rect, vec2 point) {
	return point * (rect.zw - rect.xy) + rect.xy;
}

vec2 atlas_sample(vec4 view, vec2 uv, vec2 atlas_size) {
	return point_in_rect(view, uv) / atlas_size;
}

struct Glyph {
	uvec4 view;
};

struct Text {
	mat4 transform;
	vec4 color;
};

struct Character {
	vec4 rect;
	uint glyph;
	uint text;
	uint padding[2];
};

layout(location = 0) smooth pass vec2 uv;
layout(location = 1) flat pass uvec4 view;
layout(location = 2) flat pass uint text;

layout(binding = 0) uniform sampler2D font;
layout(std430, binding = 0) buffer Characters { Character instances[]; };
layout(std430, binding = 1) buffer Texts { Text texts[]; };
layout(std430, binding = 2) buffer Glyphs { Glyph glyphs[]; };

layout(std140, binding = 0) uniform Scene {
	mat4 project;
	uvec2 font_atlas_size;
	float alpha_discard;
};

layout(std140, binding = 1) uniform Debug {
	uint frag_mode;
};

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 vert_uv;

void main() {
	uv = vert_uv;
	uint glyph = instances[gl_InstanceID].glyph;
	view = glyphs[glyph].view;
	text = instances[gl_InstanceID].text;
	//TODO rework this renderer to generate vertex data for characters instead of relying solely on instancing
	vec2 text_local_pos = point_in_rect(instances[gl_InstanceID].rect, position + vec2(0.5));//* +0.5 needed because position is -0.5->+0.5 instead of 0->1 (this is probably one of the reasons why its probably better to generate vertex data for characters instead of relying solely on instance data stuff)
	gl_Position = project * texts[text].transform * vec4(text_local_pos, 0, 1);
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;


const uint frag_mode_alpha_blend = 0;
const uint frag_mode_alpha_threshold = 1;
const uint frag_mode_sdf_threshold = 2;

void main() {

	switch (frag_mode) {
		case frag_mode_alpha_blend: {
			pixel_color = texts[text].color * texture(font, atlas_sample(view, uv, vec2(font_atlas_size))).r;
		} break;
		case frag_mode_alpha_threshold: {
			if (texture(font, atlas_sample(view, uv, vec2(font_atlas_size))).r > 0.5)
				pixel_color = texts[text].color;
			else
				discard;
		} break;
		case frag_mode_sdf_threshold: {
			if (texture(font, atlas_sample(view, uv, vec2(font_atlas_size))).r >= 0)
				pixel_color = texts[text].color;
			if (pixel_color.a < alpha_discard)
				discard;
		} break;
	}

	if (pixel_color.a < alpha_discard)
		discard;
	// pixel_color = vec4(1, 0, 1, 1);
}

#endif
