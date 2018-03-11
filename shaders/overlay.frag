#version 330 core
// for sampler bindings.
#extension GL_ARB_shading_language_420pack: require

in vec3 texcoord;
in vec3 ws_pos;
in vec3 ws_norm;
in vec4 out_color;

layout(std140, binding=0) uniform per_camera {

	mat4 view_proj_matrix;
    mat4 inv_centered_view_proj_matrix;
    float aspect;
    float time;

};

layout(location=0) out vec4 color;

float q = 16.0;

void main(void)
{
    color = out_color;

     vec3 quantized_pos = round(ws_pos * q) / q;

    float f0 = sin(3 * (0.5 * time + quantized_pos.x + quantized_pos.y));
    float f1 = sin(8 * (0.7 * time + quantized_pos.y - quantized_pos.z));
    float f2 = sin(4 * (0.9 * time + quantized_pos.z));    
    color *= max(0.2, (0.3 + 0.7 * f0 - f1 * f2));
}

