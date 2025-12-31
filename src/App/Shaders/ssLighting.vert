#version 330

layout(location=0) in vec2 pos;
layout(location=1) in vec2 texCoord;

uniform float gAspectRatio;
uniform float gTanHalfFOV;

out vec2 tex_coord;
out vec3 view_ray;

void main()
{
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    tex_coord = texCoord;
    view_ray.x = pos.x * gAspectRatio * gTanHalfFOV;
    view_ray.y = pos.y * gTanHalfFOV;
    view_ray.z = -1;
}