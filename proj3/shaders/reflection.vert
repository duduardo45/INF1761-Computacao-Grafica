#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_texCoord;

out vec2 v_texCoord;

layout (std140) uniform CameraMatrices {
    mat4 u_view;
    mat4 u_projection;
};

uniform mat4 u_model;

void main()
{
    v_texCoord = a_texCoord;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
