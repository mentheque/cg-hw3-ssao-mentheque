#version 330

// Lighting controls: 
// -----------------
#define _directionalSize 3
#define _spotSize 4
// -----------------

in vec2 tex_coord;
in vec3 view_ray;

out vec4 out_col;

uniform sampler2D diffuseTex;
uniform sampler2D depthTex;
uniform sampler2D normalTex;

uniform mat4 invView;
uniform float viewZConstants[2];

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
    float z_ndc = 2 * texture(depthTex, coords).r - 1;
    return viewZConstants[0] / (z_ndc + viewZConstants[1]);
}

vec3 get_world_pos(vec3 view_pos){
    vec4 world_pos = invView * vec4(view_pos, 1.0);
    return world_pos.xyz / world_pos.w;
}


void main()
{

    vec3 view_pos = view_ray * calc_viewZ(tex_coord);
    vec3 frag_pos_world = (invView * vec4(view_pos, 1.0)).xyz;
    vec3 norm = texture(normalTex, tex_coord).xyz;
    vec4 base_color = texture(diffuseTex, tex_coord).rgba;

    // geting information on light models
    bool spot = false;
    int skip_idx = -1;
    if(base_color.a != 1.0){
        skip_idx = int(base_color.a * 255.0) - 1;
        if(skip_idx >= _directionalSize){
            spot = true;
            skip_idx -= _directionalSize;
        }

        if(base_color.rgb == vec3(0.0)){
            if(spot){
                out_col = vec4(spotlights[skip_idx].directional.color.rgb, 1.0);
            }else{
                out_col = vec4(directionals[skip_idx].color.rgb, 1.0);
            }
            return;
        }
    }

    
    float ambient;
    float diffuse;
    float specular;

    vec3 lightAccumulare = vec3(0.0, 0.0, 0.0);

    vec3 viewDir = normalize(cameraPosWorld - frag_pos_world);

    for(int i = 0;i < _directionalSize;i++){
        if(!spot && i == skip_idx){
            continue;
        }

        ambient = directionals[i].ambientStrength;
        diffuse = directionals[i].diffuseStrength * max(dot(norm, -directionals[i].direction), 0.0);
        // 32 should be from material
        specular = directionals[i].specularStrength * 
            pow(max(dot(viewDir, reflect(directionals[i].direction, norm)), 0.0), 32);

        lightAccumulare +=  directionals[i].color * (ambient + diffuse + specular);
    }

    float edgeFadeoutAdjust = 1.0;

    for(int i = 0;i < _spotSize;i++){
        if(spot && i == skip_idx){
            continue;
        }

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

    //out_col = vec4(-viewZ, 0.0, 0.0, 1.0);
    //out_col = base_color;
}