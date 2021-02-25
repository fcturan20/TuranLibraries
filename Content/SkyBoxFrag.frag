#version 450
layout(location = 0) in vec3 TextCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outExtraColor;

layout(binding = 0, set = 1) uniform usamplerCube SkyBoxTexture;


void main() {
    uvec4 sampled =  texture(SkyBoxTexture, TextCoord);

    outColor = vec4(float(sampled.x/255.0f), float(sampled.y/255.0f), float(sampled.z/255.0f), 1.0f);
}