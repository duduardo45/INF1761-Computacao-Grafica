#version 410 core

out vec4 FragColor;

in vec3 v_fragPos;
in vec3 v_normal;

uniform vec3 u_viewPos; // posição da câmera (em espaço do mundo)

// Estrutura para a luz direcional
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight u_dirLight;

// Estrutura para o material
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material u_material;

void main()
{
    // Normal do fragmento
    vec3 norm = normalize(v_normal);

    // Direção da luz (da luz para o fragmento)
    vec3 lightDir = normalize(-u_dirLight.direction);

    // ======== COMPONENTE DIFUSA ========
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = u_dirLight.diffuse * (diff * u_material.diffuse);

    // ======== COMPONENTE ESPECULAR ========
    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_material.shininess);
    vec3 specular = u_dirLight.specular * (spec * u_material.specular);

    // ======== COMPONENTE AMBIENTE ========
    vec3 ambient = u_dirLight.ambient * u_material.ambient;

    // ======== RESULTADO FINAL ========
    vec3 result = ambient + diffuse + specular;


    FragColor = vec4(result, 1.0);
}