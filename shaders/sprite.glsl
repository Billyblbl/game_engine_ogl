
uniform sampler2D textures[MAX_TEXTURE_IMAGE_UNITS];

layout(std140) uniform Scene {
	mat4 view_matrix;
	float alpha_discard;
};

smooth pass vec2 texture_uv;
flat pass uint texture_id;
smooth pass vec4 color;

#ifdef VERTEX_SHADER

in vec2 position;
in vec2 vert_uv;
in uint albedo_index;
in float depth;
in vec4 tint;

void main() {
	uint transform_index = gl_BaseInstance//* hijacked for this purpose since this shader ins't meant to be used with instancing
	gl_Position = view_matrix * vec4(position, depth, 1.0);
	texture_uv = vert_uv;
	texture_id = albedo_index;
	color = tint;
}

#endif

#ifdef FRAGMENT_SHADER

out vec4 pixel_color;

void main() {
	pixel_color = color * texture(textures[texture_id], texture_uv);
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
