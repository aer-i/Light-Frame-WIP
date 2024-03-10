#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D fontTextures[];

layout(location = 0) in vec2      inUv;
layout(location = 1) in vec4      inColor;
layout(location = 2) in flat uint inTextureIndex;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = inColor * texture(fontTextures[nonuniformEXT(inTextureIndex)], inUv);
}