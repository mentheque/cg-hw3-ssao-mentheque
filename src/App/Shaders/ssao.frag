#version 330 core

in vec2 tex_coord;
in vec3 view_ray;

uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;

uniform mat3 normalMatrix;
uniform mat4 projection;

const int MAX_KERNEL_SIZE = 64;
uniform vec3 sampleKernel[MAX_KERNEL_SIZE];
uniform int sampleSize;
uniform float kernelRadius;
uniform float bias;

uniform vec2 noiseScale;
uniform float viewZConstants[2];

layout (location = 0) out float out_occlusion;

float calc_viewZ(vec2 coords)
{
    float z_ndc = 2 * texture(depthTex, coords).r - 1;
    return viewZConstants[0] / (z_ndc + viewZConstants[1]);
}

void main() {
    vec3 norm = normalize(normalMatrix * texture(normalTex, tex_coord).rgb);
    vec3 view_pos = view_ray * calc_viewZ(tex_coord);
    vec3 random_vec = normalize(texture(noiseTex, tex_coord * noiseScale).xyz);
    
    vec3 tangent = normalize(random_vec - norm * dot(random_vec, norm));
    vec3 bitangent = cross(norm, tangent);
    mat3 TBN = mat3(tangent, bitangent, norm);

    float occlusion = 0;
    for(int i = 0;i < sampleSize;i++){
        vec3 sample_pos = view_pos + (TBN * sampleKernel[i]) * kernelRadius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xy /= offset.w; // perspective divide
        offset.xy  = offset.xy * 0.5 + 0.5;

        // I have no clue why flipping the sign here works, but I found it in one of the 
        // comments in like the most og post, and I'm glad it finally works lol
        // Ah, it's because view_ray.z == -1
        float sample_z = -calc_viewZ(offset.xy);
        float range_check = smoothstep(0.0, 1.0, kernelRadius / abs(view_pos.z - sample_z));
        occlusion += (sample_z >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;
    }
    occlusion = 1.0 - (occlusion / sampleSize);
    out_occlusion = occlusion;
}