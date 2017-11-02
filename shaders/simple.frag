#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;

layout(binding=0) uniform sampler2DArray s_albedo;

layout(location=0) out vec4 color;

const float ambientAmount = 0.1;
const float light_step = -0.5;
const float light_pos_quantize_factor = 4;

void main(void)
{
    float light = 1.0;

	color = texture(s_albedo, texcoord) * light;

    if (color.a == 0)
        discard;
}
