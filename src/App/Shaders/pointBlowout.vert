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

uniform vec3 blowOutPoint;
uniform float radius;
uniform float transition;

void main() {
    vec3 target_normal = normalize(pos - blowOutPoint);
    vec3 target_pos = blowOutPoint + target_normal * radius;

    vec3 target_bitangent = cross(target_normal, tangent.xyz);
    vec3 target_tangent = normalize(cross(target_bitangent, target_normal));

    vec3 int_normal = normalize(mix(normal, target_normal, transition));
    vec3 int_pos = mix(pos, target_pos, transition);
    vec3 int_tang = normalize(mix(tangent.xyz, target_tangent, transition));

    N = normalize(normalMatrix * int_normal);
    T = normalize(normalMatrix * int_tang.xyz);
    handednes = tangent.w; // handednes from actual tangent
    
    T = normalize(T - dot(T, N) * N);

    //TBN = mat3(T, B, N);
    tex_coord = texCoord;
    frag_normal = normalize(normalMatrix * int_normal);


    vec4 world_pos = model * vec4(int_pos, 1.0);
    frag_pos_world = world_pos.xyz;

    gl_Position = mvp * vec4(int_pos, 1.0);
}
