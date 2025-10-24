#version 410

layout (location=0) in vec4 vertex;
layout (location=1) in vec2 textureCoord;

out vec2 vertexTexCoord;

layout (std140) uniform CameraMatrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

void main (void)
{
  vertexTexCoord = textureCoord;
  gl_Position = projection * view * u_model * vertex;
}

