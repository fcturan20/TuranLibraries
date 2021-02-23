#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTextCoord;

layout(location = 0) out vec2 TextCoord;
layout(location = 1) out vec3 MatTypeColor;


layout(binding = 0, set = 1) uniform FirstUniformInput{
    vec3 Color;
} general_buf;

void main() {
    //gl_Position = matrixes.camera_toprojection * matrixes.world_tocamera * matrixes.object_toworld *  vec4(inPosition, 0.0, 1.0);
    gl_Position = vec4(inPosition, 0.0, 1.0);
    gl_Position.y = -gl_Position.y;
    TextCoord = inTextCoord;
    MatTypeColor = general_buf.Color;
}