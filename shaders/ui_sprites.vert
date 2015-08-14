#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;
layout(location=1) in vec2 uv;


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;
    mat4 inv_centered_view_proj_matrix;
    float aspect;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix;

};


out vec2 texcoord;


void main(void)
{
    gl_Position = vec4(pos.xy * (1/256.0), 0, 1);
    gl_Position.x /= aspect;
    texcoord = uv;
}

