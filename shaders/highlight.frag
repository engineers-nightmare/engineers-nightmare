#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;

layout(binding=2) uniform sampler1D s_palette;

layout(location=0) out vec4 color;

float q = 3.0;

void main(void)
{
    vec3 quantized_pos = round(ws_pos * q) / q;
    vec3 light_dir = -normalize(quantized_pos - vec3(3.0, 2.0, 3.0));
    float light = clamp(dot(light_dir, ws_norm), 0.2, 1.0);

    color = texture(s_palette, texcoord.x) * light * 1.2;
    color.a = 0.8;

    if (color.a == 0)
        discard;
}
