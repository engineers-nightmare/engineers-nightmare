#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 ws_pos;
in float opacity;

layout(binding=0) uniform sampler2DArray s_albedo;
layout(binding=1) uniform sampler3D s_light_field;

layout(location=0) out vec4 color;

const float ambientAmount = 0.1;
const float light_pos_quantize_factor = 4;

void main(void)
{
    /* lighting */
    /* quantize for stylized look */
    vec3 light_lookup_pos = round(ws_pos * light_pos_quantize_factor) / light_pos_quantize_factor;
    float light = mix(
            texture(s_light_field, light_lookup_pos / textureSize(s_light_field, 0)).x,
            1.0,
            ambientAmount);

    color = texture(s_albedo, vec3(gl_PointCoord, 15)) * light * opacity;

    if (color.a == 0)
        discard;
}

