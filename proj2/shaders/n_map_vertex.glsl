#version 410 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec3 a_tangent;
layout (location = 3) in vec2 a_texCoord;

out vec3 v_fragPos;
out vec2 v_texCoord;
out mat3 v_TBN;

// === blocos uniformes ===
layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main()
{
    // posição do fragmento em espaço do mundo
    v_fragPos = vec3(u_model * vec4(a_pos, 1.0));

    // normal e tangente em espaço do mundo
    mat3 normalMatrix = transpose(inverse(mat3(u_model)));
    vec3 N = normalize(normalMatrix * a_normal);
    vec3 T = normalize(normalMatrix * a_tangent);
    vec3 B = normalize(cross(N, T));

    // matriz TBN: transforma do espaço tangente → mundo
    v_TBN = mat3(T, B, N);

    v_texCoord = a_texCoord;

    gl_Position = projection * view * u_model * vec4(a_pos, 1.0);
}
