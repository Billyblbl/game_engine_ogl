
struct Sheet {
	vec4 tint;
	uint texture_id;
	float depth;
};

uniform sampler2D textures[MAX_TEXTURE_IMAGE_UNITS];

layout (std430) restrict readonly buffer Sheets { Sheet sheets[]; };

layout(std140) uniform Scene {
	mat4 canvas_proj;//* based on screen dimensions
	float alpha_discard;
};

vec2 point_in_rect(vec4 rect/*xy=min zw=max*/, vec2 point) {
	return point * (rect.zw - rect.xy) + rect.xy;
}

vec2 uv_map(vec2 space, vec4 rect, vec2 uv) {
	return point_in_rect(rect, uv) / space;
}

smooth pass vec2 uv;
flat pass vec4 _color;
flat pass uint _texture;

#ifdef VERTEX_SHADER

//* Quad { <- instanced vertex attributes
	in vec4 rect;
	in uvec4 sprite;

	const vec2 verts[] = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1) };
	const vec2 uvs[] = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };
//* }

void main() {
	Sheet sheet = sheets[gl_DrawID];
	uv = uv_map(textureSize(textures[sheet.texture_id], 0), sprite, uvs[gl_VertexID % 4]);
	vec2 position = point_in_rect(rect, verts[gl_VertexID % 4]);
	gl_Position = canvas_proj * vec4(position, sheet.depth, 1);

	_color = sheet.tint;
	_texture = sheet.texture_id;
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 pixel_color;

void main() {
	pixel_color = _color * texture(textures[_texture], uv).r;
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
