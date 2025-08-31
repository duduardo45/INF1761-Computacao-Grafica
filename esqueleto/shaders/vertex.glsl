#version 410

in vec4 coords;
out vec4 fragColor;

void main() {
    gl.Position = coords;
    fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
}