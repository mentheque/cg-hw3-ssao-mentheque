#version 330 core

in vec2 tex_coord;

uniform sampler2D occlusionTex;
uniform sampler2D diffuseTex;

uniform bool showOcclusion;

out vec4 out_col;

void main() {
    if(showOcclusion){
        float occlusion = texture(occlusionTex, tex_coord).r;
        out_col = vec4(occlusion, occlusion, occlusion, 1.0);
    }else{
        out_col = vec4(texture(diffuseTex, tex_coord).rgb, 1.0);
    }
}