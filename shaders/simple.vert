#version 330 core

// for uniform block bindings.
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec4 pos;


layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};


layout(std140, binding=1) uniform per_object {

	mat4 world_matrix;

};


void main(void)
{
	gl_Position = view_proj_matrix * world_matrix * pos;
}
