#version 450
layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 TextCoord;

layout(set = 0, binding = 1) uniform WorldData{
    mat4 object_toworld;
    mat4 world_tocamera;
    mat4 sky_tocamera;
    mat4 camera_toprojection;
} CameraData;

void main() {
    gl_Position = CameraData.camera_toprojection * CameraData.sky_tocamera * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition.xy, 0.0, 1.0);
    //gl_Position.y = -gl_Position.y;
    TextCoord = inPosition;
}