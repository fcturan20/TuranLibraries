#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 vertexColor;
layout(location = 1) in vec2 textCoord;

layout(set = 1, binding = 0) uniform utexture2D firstImage;
layout(set = 2, binding = 0) uniform sampler firstSampler;

layout( push_constant ) uniform constants
{
  uint values[32];
} pushConstant;

void main() {
  uint v = 0;
  for(uint i = 0; i < 32; i++){
    v += pushConstant.values[i];
  }
    outColor = (vec4(texture(usampler2D(firstImage, firstSampler), textCoord)) + vec4(v)) / vec4(255.0f);
}