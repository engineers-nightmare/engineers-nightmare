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

	mat4 world_matrix;

};


out vec3 texcoord;
out vec3 ws_pos;
out vec3 ws_norm;
out flat int ws_mat;

void main(void)
{
    vec4 world_pos = world_matrix * pos;
    vec4 output_pos = view_proj_matrix * world_pos;
    output_pos.z -= 0.01/max(0.01,output_pos.w);
    gl_Position = output_pos;
    texcoord.z = mat;

    vec3 n = normalize(mat3(world_matrix) * norm);

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
    ws_mat = mat;
}

