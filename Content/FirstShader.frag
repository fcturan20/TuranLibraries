#version 450
layout(location = 0) in vec2 TextCoord;
layout(location = 1) in vec3 MatTypeColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outExtraColor;

layout(binding = 1, set = 1, rgba8ui) uniform uimage2D FirstSampledTexture;

layout(binding = 0, set = 1) uniform FirstUniformInput{
    vec3 Color;
} general_buf;

void main() {
    ivec2 ImageSize0 = imageSize(FirstSampledTexture);
    ivec2 SampleCoord0 = ivec2(TextCoord.x * ImageSize0.x, TextCoord.y * ImageSize0.y);
    uvec4 sampled0 = imageLoad(FirstSampledTexture, SampleCoord0);

    outColor = vec4(float(sampled0.x/255.0f), float(sampled0.y/255.0f), float(sampled0.z/255.0f), 1.0f);
    outExtraColor = vec4(general_buf.Color, 1.0f);
}