#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;
layout(location=1) in int mat;
layout(location=2) in vec3 norm;


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix[256];

};


out vec3 texcoord;
out vec3 ws_pos;
out vec3 ws_norm;

void main(void)
{
	mat4 world_mat = world_matrix[gl_InstanceID];

    vec4 world_pos = world_mat * pos;
	gl_Position = view_proj_matrix * world_pos;
    texcoord.z = mat;

    vec3 n = normalize(mat3(world_mat) * norm);

    /* Quick & dirty triplanar mapping */
    if (n.x > 0.8) {
        texcoord.xy = vec2(-world_pos.y, -world_pos.z);
	} else if (n.x < -0.8) {
        texcoord.xy = vec2(world_pos.y, -world_pos.z);
    } else if (n.y > 0.8) {
        texcoord.xy = vec2(world_pos.x, -world_pos.z);
	} else if (n.y < -0.8) {
		texcoord.xy = vec2(-world_pos.x, -world_pos.z);
    } else if (n.z < -0.8) {
		texcoord.xy = vec2(world_pos.x, -world_pos.y);
	} else {
        texcoord.xy = world_pos.xy;
    }

    ws_pos = world_pos.xyz;
    ws_norm = n;
}
