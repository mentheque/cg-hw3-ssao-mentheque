#version 330 core

in vec2 tex_coord;

uniform sampler2D diffuseText;
uniform sampler2D depthTex;
uniform sampler2D normalTex;


layout (location = 0) out vec4 out_col;

void main() {
    vec3 norm = texture(normalTex, tex_coord).rgb;
    if(norm.r == 0 && norm.g == 0 && norm.b == 0){
        out_col = vec4(1.0, 0.0, 0.0, 1.0);
    }else{    
        out_col = vec4(texture(normalTex, tex_coord).rgb*0.5 + 0.5, 1.0);
    }
}