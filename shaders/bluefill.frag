#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require
#extension GL_ARB_explicit_uniform_location: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;
in flat int ws_mat;

layout(binding=0) uniform sampler2DArray s_albedo;
layout(binding=1) uniform sampler3D s_light_field;
layout(location=0) uniform float time;

layout(location=0) out vec4 color;

const float ambientAmount = 0.1;
const float light_step = -0.5;
const float light_pos_quantize_factor = 4;

void main(void)
{
    //if((ws_mat == 4 || ws_mat == 1))
    if((dot(ws_pos, vec3(1.0, 0.2, -0.5)) - dot(pow(fract(ws_pos*2.0)*2.0-1.0,vec3(2.0)), vec3(1.0)) < (ws_mat == 1 ? 4.0 : 8.0) - time*2.0))
        discard;

    /* lighting */
    vec3 light_lookup_pos = (ws_pos + light_step * ws_norm);
    /* quantize for stylized look */
    light_lookup_pos = round(light_lookup_pos * light_pos_quantize_factor) / light_pos_quantize_factor;
    float light = mix(
        texture(s_light_field, light_lookup_pos / textureSize(s_light_field, 0)).x,
        1.0,
        ambientAmount);

    color = texture(s_albedo, texcoord) * light;

    if (color.a == 0.0)
    discard;
}
