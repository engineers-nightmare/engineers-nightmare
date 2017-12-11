#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;
layout(location=2) in vec3 norm;


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix;
	int material;

};


out vec3 texcoord;
out vec3 ws_pos;
out vec3 ws_norm;

void main(void)
{
    vec4 world_pos = world_matrix * pos;
	gl_Position = view_proj_matrix * world_pos;
    texcoord.z = material;

    vec3 n = normalize(mat3(world_matrix) * norm);

    /* Quick & dirty triplanar mapping */
    if (norm.x > 0.8) {
	    texcoord.xy = vec2(-pos.y, -pos.z);
    } else if (norm.x < -0.8) {
	    texcoord.xy = vec2(pos.y, -pos.z);
    } else if (norm.y > 0.8) {
	    texcoord.xy = vec2(pos.x, -pos.z);
    } else if (norm.y < -0.8) {
	    texcoord.xy = vec2(-pos.x, -pos.z);
    } else if (norm.z < -0.8) {
	    texcoord.xy = vec2(pos.x, -pos.y);
    } else {
	    texcoord.xy = pos.xy;
    }

    ws_pos = world_pos.xyz;
    ws_norm = n;
}
