#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;

layout(binding=2) uniform sampler1D s_palette;

layout(location=0) out vec4 color;

void main(void)
{
    float light = 1.0;

	color = texture(s_palette, texcoord.x) * light;

    if (color.a == 0)
        discard;
}
