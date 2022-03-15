#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextCoord;

layout(location = 0) out vec2 TextCoord;

layout(set = 0, binding = 1) uniform WorldData{
    mat4 object_toworld;
    mat4 world_tocamera;
    mat4 sky_tocamera;
    mat4 camera_toprojection;
    mat4 camera_toworld;
    vec3 camera_worldpos;
} CameraData;

void main() {
    gl_Position = CameraData.camera_toprojection * CameraData.world_tocamera * CameraData.object_toworld *  vec4(inPosition.x, -inPosition.y, inPosition.z, 1.0);
    //gl_Position = vec4(inPosition, 1.0);
    TextCoord = inTextCoord;
}