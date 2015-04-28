#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix;

};


out vec2 texcoord;
out vec4 tint;


void main(void)
{
    gl_Position = vec4(pos.xy * (1/512.0), 0, 1);
    texcoord = uv;
    tint = color;
}
