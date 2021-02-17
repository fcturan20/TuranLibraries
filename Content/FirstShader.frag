#version 450
layout(location = 0) in vec2 TextCoord;
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outExtraColor;

layout(binding = 1, set = 2) uniform usampler2D FirstSampledTexture;

void main() {
    uvec4 sampled = texture(FirstSampledTexture, TextCoord);
    outColor = vec4(float(sampled.x/255.0f), float(sampled.y/255.0f), float(sampled.z/255.0f), 1.0f);
    outExtraColor = vec4(TextCoord, 0, 255);
}