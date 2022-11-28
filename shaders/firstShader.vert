#version 450

vec2 positions[6] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5)
);
vec2 textCoords[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0)
);
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(std140, set = 0, binding = 0) readonly buffer transform{
    vec2 translate;
};

layout(location = 0) out vec3 vertexColor;
layout(location = 1) out vec2 textCoord;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] + translate, 0.0, 1.0);
    textCoord = textCoords[gl_VertexIndex];
    vertexColor = colors[gl_VertexIndex];
}