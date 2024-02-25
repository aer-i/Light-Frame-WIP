#version 460

layout(binding = 1) uniform sampler2D fontTexture;

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0, 1, 0.5, texture(fontTexture, inUv).r);
}