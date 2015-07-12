#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;
    mat4 inv_centered_view_proj_matrix;

};


out vec3 view_dir;


void main(void)
{
    gl_Position = vec4(-1, -1, 0.999999, 1);
    if (gl_VertexID == 2) gl_Position.x += 4;
    if (gl_VertexID == 1) gl_Position.y += 4;

    view_dir = (inv_centered_view_proj_matrix * gl_Position).xyz;
}

