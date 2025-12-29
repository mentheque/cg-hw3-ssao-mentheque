#version 330 core

struct ShineThrough{
    int idx;
};

uniform ShineThrough shineThrough;

/*
    So this is just model.frag copied out here
    with some minor changes to uniforms and handling
    of pitch black fragments. Could this code have been 
    written to the main lighting shader? Perhaps. 
    Maybe it actually should have, but this for now.
*/

in vec3 frag_normal;
in vec2 tex_coord;

uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;

in vec3 frag_pos_world;

in vec3 N;
in vec3 T;
in float handednes;

struct Material{
    vec4 base_color;
    bool has_texture;
    bool has_normalMap;
};
uniform Material material;

layout (location = 0) out vec4 out_diffuse;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_position;


vec4 sample_frag_color(){
    vec4 tex_color = texture(diffuse_texture, tex_coord);
    tex_color.a = 1.0;

    vec4 base_color = material.base_color;

    if(material.has_texture){
        base_color *= tex_color;
    }

    return base_color;
}

void main() {
    vec3 norm = normalize(frag_normal);
    if(material.has_normalMap){
        vec3 fN = normalize(N);
        vec3 fT = normalize(T);
        fT = normalize(fT - dot(fT, fN) * fN);
        vec3 fB = normalize(cross(fN, fT)) * handednes;

        mat3 TBN = mat3(fT, fB, fN);

        norm = texture(normal_texture, tex_coord).rgb;
        norm = normalize(norm * 2.0 - 1.0);   
        norm = normalize(TBN * norm);
    }

    out_diffuse = sample_frag_color();
    out_normal = norm;
    out_diffuse.a = (float(shineThrough.idx)) / 255.0;
    out_position = frag_pos_world;
}