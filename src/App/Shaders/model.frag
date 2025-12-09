#version 330 core

//in vec3 frag_pos;
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

out vec4 out_col;

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

    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(camera_pos_world - frag_pos_world);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.5;
    //vec3 viewDir = normalize(viewPos - fragPos);
    //vec3 reflectDir = reflect(-lightDir, norm);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    //vec3 specular = specularStrength * spec * lightColor;

    vec4 tex_color = texture(diffuse_texture, tex_coord);

    vec4 base_color = material.base_color;

    if(material.has_texture){
        base_color = tex_color;
    }

    vec3 result = (ambient + diffuse) * base_color.rgb;
    out_col = vec4(result, base_color.a);
    //out_col = vec4(norm.rgb, 1.0);
}