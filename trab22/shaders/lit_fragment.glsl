#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;

// Maximum number of lights - must match C++ light_config.h
#define MAX_SCENE_LIGHTS 16

// Light type enumeration - must match C++ LightType enum
#define LIGHT_TYPE_INACTIVE 0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT 2
#define LIGHT_TYPE_SPOT 3

struct LightData {
    vec4 position;      // World-space position (w=1), unused for directional
    vec4 direction;     // World-space direction (w=0), unused for point
    vec4 ambient;       // Ambient color (RGB + padding)
    vec4 diffuse;       // Diffuse color (RGB + padding)
    vec4 specular;      // Specular color (RGB + padding)
    vec4 attenuation;   // (constant, linear, quadratic, cutoff_angle)
    int type;           // LightType enum value
    int padding1;       // 4 bytes (manual padding)
    int padding2;       // 4 bytes (manual padding)
    int padding3;       // 4 bytes (manual padding)
};

// Scene lights uniform block with std140 layout
layout(std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

// Material uniforms - set by the material system
// Note: GLSL struct notation in uniform names (u_material.ambient) is just a naming convention
// These are actually individual uniforms, not a struct
uniform vec3 u_material_ambient;
uniform vec3 u_material_diffuse;
uniform vec3 u_material_specular;
uniform float u_material_shininess;

void main()
{
    // Get the first light (point light)
    LightData u_dirLight = sceneLights.lights[0];

    // Fragment normal
    vec3 norm = normalize(v_normal);
    
    // ======== DIRECTION CALCULATION ========
    // Direction vector from fragment position TO the light
    vec3 toLightVector = u_dirLight.position.xyz - v_fragPos;
    
    // Normalized direction for lighting calculations
    vec3 lightDir = normalize(toLightVector);

    // ======== DIFFUSE COMPONENT ========
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = u_dirLight.diffuse.xyz * (diff * u_material_diffuse);

    // ======== SPECULAR COMPONENT ========
    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_material_shininess);
    vec3 specular = u_dirLight.specular.xyz * (spec * u_material_specular);

    // ======== AMBIENT COMPONENT ========
    vec3 ambient = u_dirLight.ambient.xyz * u_material_ambient;

    // ======== ATENUAÇÃO (CRÍTICO PARA LUZ DE PONTO) ========
    // Sem isso, a luz terá a mesma intensidade em toda a cena.
    // Usamos o toLightVector *antes* de normalizar para obter a distância
    float dist = length(toLightVector);
    float attenuation = 1.0 / (u_dirLight.attenuation.x + 
                                 u_dirLight.attenuation.y * dist + 
                                 u_dirLight.attenuation.z * dist * dist);

    // ======== RESULTADO FINAL ========
    vec3 result = ambient + (diffuse + specular) * attenuation;

    FragColor = vec4(result, 1.0);
}