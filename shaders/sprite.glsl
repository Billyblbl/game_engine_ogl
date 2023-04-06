
struct rtf32 {
	vec2 min_corner;
	vec2 max_corner;
};

struct InstanceData {
	mat4 matrix;
	rtf32 uv_rect;
	uvec2 depths; //x -> altas page, y -> sprite depths
};

vec2 rect_to_world(rtf32 rect, vec2 v) {
	return v * (rect.max_corner - rect.min_corner) + rect.min_corner;
}

#ifdef VERTEX_SHADER
#define pass out
#endif

#ifdef FRAGMENT_SHADER
#define pass in
#endif

layout(location = 0) smooth pass vec2 frag_uv;
layout(location = 1) flat pass uint instance;

layout(binding = 0) uniform sampler2DArray atlas;
layout(std430, binding = 0) buffer Entities { InstanceData instances[]; };
layout(std140, binding = 0) uniform Scene { mat4 view_matrix; };

#ifdef VERTEX_SHADER

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

void main() {
	gl_Position = view_matrix * instances[gl_InstanceID].matrix * vec4(position, instances[gl_InstanceID].depths.y, 1.0);
	frag_uv = rect_to_world(instances[gl_InstanceID].uv_rect, uv);
	instance = gl_InstanceID;
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) out vec4 pixel_color;

const float alpha_discard = 0.01;

void main() {
	pixel_color = texture(atlas, vec3(frag_uv, instances[instance].depths.x));
	if (pixel_color.a < alpha_discard)
		discard;
}

#endif
