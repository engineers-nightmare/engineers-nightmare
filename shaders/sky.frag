#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require


layout(binding=0) uniform samplerCube s_sky;


in vec3 view_dir;


layout(location=0) out vec4 color;


void main(void)
{
    color = texture(s_sky, view_dir);
}


