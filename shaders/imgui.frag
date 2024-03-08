#version 460

layout(binding = 1) uniform sampler2D fontTexture;

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = inColor * texture(fontTexture, inUv);
}