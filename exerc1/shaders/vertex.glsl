#version 410

in vec4 coords;
out vec4 vertexColor;

void main() {
    gl_Position = coords;
    vertexColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
}