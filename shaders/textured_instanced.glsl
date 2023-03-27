#ifdef VERTEX_SHADER

// vertex layout
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

// buffer objects
layout(std140, binding = 0) uniform Scene { mat4 viewMatrix; };
layout(std430, binding = 0) buffer Matrices { mat4 matrices[]; };

// passing to frag
layout(location = 0) smooth out vec2 outUV;

void main() {
	gl_Position = viewMatrix * matrices[gl_InstanceID] * vec4(position, 0.0, 1.0);
	outUV = uv;
}

#endif

#ifdef FRAGMENT_SHADER

layout(location = 0) smooth in vec2 uv;
layout(binding = 0) uniform sampler2D texture;

layout(location = 0) out vec4 outColor;


const float alphaDiscard = 0.01;

void main() {
	outColor = texture2D(texture, uv);
	if (outColor.a < alphaDiscard)
		discard;
}

#endif
