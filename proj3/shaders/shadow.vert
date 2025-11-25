#version 330 core
layout (location = 0) in vec3 aPos;
// ... other attributes

uniform mat4 u_model;
uniform mat4 u_viewProjection; 
uniform mat4 u_lightSpaceMatrix; // Passed from C++

out vec4 FragPosLightSpace; // Output to fragment shader

void main() {
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    gl_Position = u_viewProjection * worldPos;
    
    // Transform vertex to light space
    FragPosLightSpace = u_lightSpaceMatrix * worldPos;
}