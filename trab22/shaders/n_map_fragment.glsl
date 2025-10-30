#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;
in vec2 v_texCoord;
in mat3 v_TBN;

// ========== Definições da cena ==========
#define MAX_SCENE_LIGHTS 16
#define LIGHT_TYPE_INACTIVE    0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_POINT       2
#define LIGHT_TYPE_SPOT        3

struct LightData {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation; // (const, linear, quad, cutoff)
    int type;
    int pad1;
    int pad2;
    int pad3;
};

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
} sceneLights;

layout (std140) uniform CameraPosition {
    vec4 u_viewPos;
};

// ========== Material e texturas ==========
uniform vec3 u_material_ambient;
uniform vec3 u_material_diffuse;
uniform vec3 u_material_specular;
uniform float u_material_shininess;

uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_roughnessMap;

// ========== Função principal ==========
void main()
{
    // recupera luz ativa (apenas a primeira, para simplificar)
    LightData u_pointLight = sceneLights.lights[0];

    // --- normal perturbada via normal map ---
    vec3 sampledNormal = texture(u_normalMap, v_texCoord).rgb;
    sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
    vec3 norm = normalize(v_TBN * sampledNormal);

    // --- posição e direção ---
    vec3 toLight = u_pointLight.position.xyz - v_fragPos;
    vec3 lightDir = normalize(toLight);
    vec3 viewDir  = normalize(u_viewPos.xyz - v_fragPos);
    vec3 halfway  = normalize(lightDir + viewDir);

    // --- componentes de iluminação ---
    float diff = max(dot(norm, lightDir), 0.0);

    // rugosidade controla brilho
    float roughness = texture(u_roughnessMap, v_texCoord).r;
    float shininess = mix(512.0, 4.0, clamp(roughness, 0.0, 1.0));

    float spec = 0.0;
    if (diff > 0.0)
        spec = pow(max(dot(norm, halfway), 0.0), shininess);

    // --- cores base do material (textura difusa) ---
    vec3 albedo = texture(u_diffuseMap, v_texCoord).rgb;

    // --- luz ambiente, difusa e especular ---
    vec3 ambient  = u_pointLight.ambient.xyz * (u_material_ambient * albedo);
    vec3 diffuse  = u_pointLight.diffuse.xyz * diff * (u_material_diffuse * albedo);
    vec3 specular = u_pointLight.specular.xyz * spec * u_material_specular;

    // --- atenuação (ponto/luz local) ---
    float dist = length(toLight);
    float attenuation = 1.0 /
        (u_pointLight.attenuation.x +
         u_pointLight.attenuation.y * dist +
         u_pointLight.attenuation.z * dist * dist);

    vec3 result = (ambient + diffuse + specular) * attenuation;

    FragColor = vec4(result, 1.0);
}
