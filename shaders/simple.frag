#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;

layout(binding=0) uniform sampler2DArray s_albedo;
layout(binding=1) uniform sampler3D s_light_field;

layout(location=0) out vec4 color;

const float ambientAmount = 0.1;
const float light_step = -0.5;
const float light_pos_quantize_factor = 4;

void main(void)
{
    /* lighting */
    vec3 light_lookup_pos = (ws_pos + light_step * ws_norm);
    /* quantize for stylized look */
    light_lookup_pos -= (fract(light_lookup_pos * light_pos_quantize_factor) / light_pos_quantize_factor);
    float light = ambientAmount + (1 - ambientAmount) * texture(s_light_field, light_lookup_pos / textureSize(s_light_field, 0)).x;

	color = texture(s_albedo, texcoord) * light;

    if (color.a == 0)
        discard;
}
