#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(std140, binding=0) uniform per_camera {

    mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

    vec4 particle_data[256];  /* pos in xyz, lifetime in w */

};

out vec3 ws_pos;
out float opacity;

void main(void)
{
    vec4 data = particle_data[gl_VertexID];

    gl_Position = view_proj_matrix * vec4(data.xyz, 1);
    gl_PointSize = (120 - 80 * data.w) / gl_Position.w;

    ws_pos = data.xyz;
    opacity = data.w * 0.7f;
}

