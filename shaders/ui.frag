#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require


in vec2 texcoord;
in vec4 tint;


layout(binding=0) uniform sampler2D s_text_mask;


layout(location=0) out vec4 color;


void main(void)
{
	color = texture(s_text_mask, texcoord).x * tint;

    if (color.a == 0)
        discard;
}

