#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 VertexNormal;

layout(binding = 0, set = 2) uniform FirstMeshTranformations{
    mat4 object_toworld;
    mat4 world_tocamera;
    mat4 camera_toprojection;
} matrixes;

void main() {
    gl_Position = matrixes.camera_toprojection * matrixes.world_tocamera * matrixes.object_toworld *  vec4(inPosition, 0.0, 1.0);
    VertexNormal = inNormal;
}