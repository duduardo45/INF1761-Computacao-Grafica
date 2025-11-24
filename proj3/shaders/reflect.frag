#version 410 core

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D uReflectionTex; // ReflectionPass registers this sampler

void main() {
    vec4 refl = texture(uReflectionTex, v_texCoord);
    // Simple output: reflection with slight fresnel-like fade by view angle could be added,
    // but keep it straightforward: use reflection texture directly
    FragColor = refl;
}
