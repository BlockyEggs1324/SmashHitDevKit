#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 vColor;
out vec2 vTexCoord;

uniform mat4 uMvpMatrix;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = uMvpMatrix * vec4(aPosition, 1.0);
}