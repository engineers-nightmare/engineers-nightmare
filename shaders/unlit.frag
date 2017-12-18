#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;
in vec4 out_color;

layout(location=0) out vec4 color;

void main(void)
{
    color = out_color;
}

