#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoord;

out vec2 v_texCoord;

// Camera matrices block (engine provides this)
layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main() {
    v_texCoord = aTexCoord;
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;
}
