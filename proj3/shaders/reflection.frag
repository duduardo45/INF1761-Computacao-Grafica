#version 410 core

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_reflectionTexture;
// Tint color and factor to blend a base tint over the reflection texture
uniform vec3 u_tintColor;
uniform float u_tintFactor; // 0.0 = no tint, 1.0 = full tint

void main()
{
    vec3 col = texture(u_reflectionTexture, v_texCoord).rgb;
    col = mix(col, u_tintColor, clamp(u_tintFactor, 0.0, 1.0));
    FragColor = vec4(col, 1.0);
}
