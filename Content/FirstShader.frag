#version 450
layout(location = 0) in vec2 TextCoord;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outExtraColor;

layout(binding = 1, set = 2, rgba8ui) uniform uimage2D FirstSampledTexture;

void main() {
    ivec2 ImageSize = imageSize(FirstSampledTexture);
    ivec2 SampleCoord = ivec2(TextCoord.x * ImageSize.x, TextCoord.y * ImageSize.y);
    uvec4 sampled = imageLoad(FirstSampledTexture, SampleCoord);
    outColor = vec4(float(sampled.x/255.0f), float(sampled.y/255.0f), float(sampled.z/255.0f), 1.0f);
    sampled = uvec4(100, 168, 32, 255);
    imageStore(FirstSampledTexture, SampleCoord, sampled);
    outExtraColor = vec4(TextCoord, 0, 255);
}