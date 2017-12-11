#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;
layout(location=2) in vec3 norm;
layout(location=3) in vec2 uv;

layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix;

};


out vec3 texcoord;
out vec3 ws_pos;
out vec3 ws_norm;

void main(void)
{
    vec4 world_pos = world_matrix * pos;
	gl_Position = view_proj_matrix * world_pos;
    texcoord.z = 0;

    vec3 n = normalize(mat3(world_matrix) * norm);
    texcoord.xy = uv;

    ws_pos = world_pos.xyz;
    ws_norm = n;
}
