#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 VertexColor;

layout(binding = 1, set = 0) uniform FirstGlobalBuffer{
    vec3 FirstVec3;
} glob_buf;

layout(binding = 0, set = 1) uniform FirstUniformInput{
    vec3 Color;
} u_input;

void main() {
    //gl_Position = matrixes.camera_toprojection * matrixes.world_tocamera * matrixes.object_toworld *  vec4(inPosition, 0.0, 1.0);
    gl_Position = vec4(inPosition, 0.0, 1.0);
    VertexColor = glob_buf.FirstVec3;
}