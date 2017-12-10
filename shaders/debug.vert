#version 330 core
#extension GL_ARB_shading_language_420pack: require

layout(location=0) in vec3 in_Position;
layout(location=1) in vec4 in_ColorPointSize;

layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;

};

out vec4 v_Color;

void main()
{
	gl_Position  = view_proj_matrix * vec4(in_Position, 1.0);
	gl_PointSize = in_ColorPointSize.w;
	v_Color      = vec4(in_ColorPointSize.xyz, 1.0);
}
