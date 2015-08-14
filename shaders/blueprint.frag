#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require
#extension GL_ARB_explicit_uniform_location: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;
in flat int ws_mat;

layout(binding=0) uniform sampler2DArray s_albedo;
layout(location=0) uniform float time;

layout(location=0) out vec4 color;

void main(void)
{
    color = texture(s_albedo, texcoord);

    if (color.a == 0.0)
        discard;

    if(ws_mat == 1)
        color.rgb *= 0.5;

    color.rgb *= color.a;
    color.rgb *= 0.5;
    color.rg *= color.rg;
    color.rgb *= 0.2/0.5;
}

