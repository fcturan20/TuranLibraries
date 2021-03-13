#version 450
layout(location = 0) in vec2 TextCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 1) uniform usampler2D SampledTextures[];


void main() {
    //ivec2 ImageSize0 = imageSize(FirstSampledTexture[1]);
    //ivec2 SampleCoord0 = ivec2(TextCoord.x * ImageSize0.x, (1.0f - TextCoord.y) * ImageSize0.y);
    //uvec4 sampled0 = imageLoad(FirstSampledTexture[1], SampleCoord0);
    uvec4 sampled0 = texture(SampledTextures[0], TextCoord);

    outColor = vec4(float(sampled0.x/255.0f), float(sampled0.y/255.0f), float(sampled0.z/255.0f), 1.0f);
}