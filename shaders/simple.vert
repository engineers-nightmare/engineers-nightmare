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
out float light;

const vec3 lightPos = vec3(4,4,4);
//const float ambientAmount = 0.3;
const float ambientAmount = 1.0;

void main(void)
{
    vec4 world_pos = world_matrix * pos;
	gl_Position = view_proj_matrix * world_pos;
    texcoord.z = mat;

    vec3 n = abs(normalize(norm));
    /* Quick & dirty triplanar mapping */
    if (n.x > 0.8) {
        texcoord.xy = pos.yz;
    } else if (n.y > 0.8) {
        texcoord.xy = pos.xz;
    } else {
        texcoord.xy = pos.xy;
    }

    /* lighting */
    vec3 light_dir = lightPos - world_pos.xyz;
    float lambert = clamp(dot(normalize(light_dir), norm), 0, 1);

    light = ambientAmount + (1 - ambientAmount) * lambert;
}
