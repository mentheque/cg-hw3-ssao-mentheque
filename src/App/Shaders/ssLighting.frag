#version 330

// Lighting controls: 
// -----------------
#define _directionalSize 3
#define _spotSize 4
// -----------------

in vec2 tex_coord;
in vec2 view_ray;

out vec4 out_col;

uniform sampler2D diffuseTex;
uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D positionTex;

uniform mat4 projection;

const int MAX_KERNEL_SIZE = 64;
uniform vec3 sampleKernel[MAX_KERNEL_SIZE];

uniform mat4 invView;
uniform mat4 invProj;
uniform float nearPlane;
uniform float farPlane;

uniform vec3 cameraPosWorld;

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

float calc_viewZ(vec2 coords)
{
    float depth = texture(depthTex, coords).r;
    return projection[3][2] / (2 * depth -1 - projection[2][2]);
}

vec3 get_world_pos(vec3 view_pos){
    vec4 world_pos = invView * vec4(view_pos, 1.0);
    return world_pos.xyz / world_pos.w;
}

float linearize_depth(float depth)
{
    float z_n = 2.0 * depth - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z_n * (farPlane - nearPlane));
}

vec3 reconstruct_world_position(vec2 texCoord, float depth, vec2 ray)
{
    float linearDepth = linearize_depth(depth);
    vec3 viewPos = vec3(ray * linearDepth, linearDepth);
    vec4 worldPos = invView * vec4(viewPos, 1.0);
    return worldPos.xyz;
}


void main()
{
    float viewZ = calc_viewZ(tex_coord);

    float viewX = view_ray.x * viewZ;
    float viewY = view_ray.y * viewZ;

    vec3 view_pos = vec3(viewX, viewY, viewZ);
    vec3 frag_pos_world = texture(positionTex, tex_coord).xyz;
    vec3 norm = texture(normalTex, tex_coord).xyz;
    vec4 base_color = texture(diffuseTex, tex_coord).rgba;

    
    float ambient;
    float diffuse;
    float specular;

    vec3 lightAccumulare = vec3(0.0, 0.0, 0.0);

    vec3 viewDir = normalize(cameraPosWorld - frag_pos_world);

    for(int i = 0;i < _directionalSize;i++){
        ambient = directionals[i].ambientStrength;
        diffuse = directionals[i].diffuseStrength * max(dot(norm, -directionals[i].direction), 0.0);
        // 32 should be from material
        specular = directionals[i].specularStrength * 
            pow(max(dot(viewDir, reflect(directionals[i].direction, norm)), 0.0), 32);

        lightAccumulare +=  directionals[i].color * (ambient + diffuse + specular);
    }

    float edgeFadeoutAdjust = 1.0;

    for(int i = 0;i < _spotSize;i++){
        vec3 fromLightDir = normalize(frag_pos_world - spotlights[i].position);
        
        float cosine = dot(fromLightDir, spotlights[i].directional.direction);

        diffuse = specular = 0;

        if(cosine > spotlights[i].outerCutoff){
            diffuse = spotlights[i].directional.diffuseStrength * 
                max(dot(norm, -spotlights[i].directional.direction), 0.0);
            // 32 should be from material
            specular = spotlights[i].directional.specularStrength * 
                pow(max(dot(viewDir, reflect(spotlights[i].directional.direction, norm)), 0.0), 32);

            // Just linear based on cosine? Looks a bit wierd but whatever
            if(cosine < spotlights[i].innerCutoff){
                edgeFadeoutAdjust = (cosine - spotlights[i].outerCutoff) 
                    / (spotlights[i].innerCutoff - spotlights[i].outerCutoff);
                diffuse *= edgeFadeoutAdjust;
                specular *= edgeFadeoutAdjust;
            }
        }
        ambient = spotlights[i].directional.ambientStrength;

        lightAccumulare += spotlights[i].directional.color * (ambient + diffuse + specular);
    }
    
    vec3 linear_out = clamp(lightAccumulare * vec3(base_color), 0.0, 1.0);
    out_col = vec4(pow(linear_out.rgb, vec3(1.0/2.2)), 1.0); // deal with alpha later
    //out_col = base_color;
}