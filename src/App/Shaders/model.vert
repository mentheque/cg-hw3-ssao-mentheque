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

void main() {
    N = normalize(normalMatrix * normal);
    T = normalize(normalMatrix * tangent.xyz);
    handednes = tangent.w;
    
    //T = normalize(T - dot(T, N) * N);

    //TBN = mat3(T, B, N);
    tex_coord = texCoord;
    frag_normal = normalMatrix * normal;
    vec4 worldPos = model * vec4(pos, 1.0);
    frag_pos_world = worldPos.xyz;

    gl_Position = mvp * vec4(pos, 1.0);
}
