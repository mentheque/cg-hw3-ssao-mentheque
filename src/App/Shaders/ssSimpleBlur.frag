#version 330 core

in vec2 tex_coord;

uniform sampler2D occlusionTex;

uniform int blurSize;

out float out_occlusion;

void main() {
    if(blurSize == 0){
        out_occlusion = texture(occlusionTex, tex_coord).r;
        return;
    }
    
    vec2 texel_size = 1.0 / vec2(textureSize(occlusionTex, 0));
    float result = 0.0;
    for (int x = -blurSize; x < blurSize; ++x) 
    {
        for (int y = -blurSize; y < blurSize; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(occlusionTex, tex_coord + offset).r;
        }
    }
    out_occlusion = result / (4.0 * blurSize * blurSize);
}  