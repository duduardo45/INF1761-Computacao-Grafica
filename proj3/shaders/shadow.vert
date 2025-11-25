#version 410 core

layout (location = 0) in vec3 aPos;

// Model matrix (Object -> World)
uniform mat4 u_model;

// Light's View * Projection matrix (World -> Clip)
// This matches the "u_lightViewProj" uniform you set in your C++ code
uniform mat4 u_lightViewProj;

void main()
{
    // Transform vertex position to light space
    gl_Position = u_lightViewProj * u_model * vec4(aPos, 1.0);
}