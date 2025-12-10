#version 330 core

// Lighting controls: 
// -----------------
#define _directionalSize 2
#define _spotSize 2
// -----------------

in vec3 frag_normal;
in vec2 tex_coord;

uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;

uniform vec3 camera_pos_world;
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

layout (std140) uniform LightSources {
    struct DirectionalLight{
        vec3 direction; // expect to be normalised.
        vec3 color;
        float ambientStrength;
        float diffuseStrength;
        float specularStrength;
    } directionals[_directionalSize];

    struct SpotLight{
        DirectionalLight directional;
        vec3 position;
        float innerCutoff;
        float outerCutoff;
    } spotlights[_spotSize];
};

out vec4 out_col;

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
    if(false /*material.has_normalMap*/){
        vec3 fN = normalize(N);
        vec3 fT = normalize(T);
        fT = normalize(fT - dot(fT, fN) * fN);
        vec3 fB = cross(fN, fT) * handednes;

        mat3 TBN = mat3(fT, fB, fN);

        norm = texture(normal_texture, tex_coord).rgb;
        norm = normalize(norm * 2.0 - 1.0);   
        norm = normalize(TBN * norm);
    }

    float ambient;
    float diffuse;
    float specular;

    vec3 lightAccumulare = vec3(0.0, 0.0, 0.0);

    vec3 viewDir = normalize(camera_pos_world - frag_pos_world);

    for(int i = 0;i < _directionalSize;i++){
        ambient = directionals[i].ambientStrength;
        diffuse = directionals[i].diffuseStrength * max(dot(norm, -directionals[i].direction), 0.0);
        // 32 should be from material
        specular = directionals[i].specularStrength * 
            pow(max(dot(viewDir, reflect(directionals[i].direction, norm)), 0.0), 32);

        lightAccumulare +=  directionals[i].color * (ambient + diffuse + specular);
    }

    vec4 base_color = sample_frag_color();
    
    vec3 linear_out = clamp(lightAccumulare * vec3(base_color), 0.0, 1.0);
    out_col = vec4(pow(linear_out.rgb, vec3(1.0/2.2)), base_color.a);
}