#version 450
layout(location = 0) in vec3 VertexColor;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outExtraColor;

void main() {
    outColor = vec4(VertexColor, 1.0f);
    outExtraColor = vec4(0.3f, 0.5f, 0.2f, 1.0f);
}