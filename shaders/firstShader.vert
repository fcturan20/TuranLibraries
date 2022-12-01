#version 450
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec4 textCoords;
layout(location = 2) in vec4 instTranslate;


layout(std140, set = 0, binding = 0) readonly buffer transform{
    vec2 positions[6];
    vec2 translate;
};

layout(location = 0) out vec3 vertColor;
layout(location = 1) out vec2 textCoord;

void main() {
    vec2 screenRatio_divisor = vec2(1.6, 0.9);
    //gl_Position = vec4((positions[gl_VertexIndex] + translate) / screenRatio_divisor, 0.0, 1.0);
    gl_Position = vec4((vertPos.xy + instTranslate.xy) / screenRatio_divisor, 0.0, 1.0);
    gl_Position.y = -gl_Position.y;
    textCoord = textCoords.xy;
    vertColor = vec3(0.0f);
}