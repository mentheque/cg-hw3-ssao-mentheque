#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;
layout(location=3) in vec4 tangent;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 frag_pos_world;
out vec3 frag_normal;
out vec2 tex_coord;
out vec3 N;
out vec3 T;
out float handednes;

uniform float radius;

void main() {
    N = normalize(normalMatrix * normal);
    T = normalize(normalMatrix * tangent.xyz);
    handednes = tangent.w;
    
    T = normalize(T - dot(T, N) * N);

    //TBN = mat3(T, B, N);
    tex_coord = texCoord;
    frag_normal = normalize(normalMatrix * normal);
    
    vec3 projected_pos = pos;
    if(pos.y < 0){
        projected_pos = normalize(vec3(pos.x, 0.0, pos.z));
        projected_pos *= radius;
        projected_pos.y = pos.y;
    }

    vec4 world_pos = model * vec4(projected_pos, 1.0);
    frag_pos_world = world_pos.xyz;

    gl_Position = mvp * vec4(projected_pos, 1.0);
}
