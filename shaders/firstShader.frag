#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 vertexColor;

void main() {
    outColor = vec4(vertexColor, 1.0);
}