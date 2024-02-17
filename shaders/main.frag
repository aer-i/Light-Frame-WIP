#version 460

layout(binding = 4) uniform sampler2D textures[];

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec3 textureColor = texture(textures[0], inUv).rgb;
    outColor = vec4(textureColor, 1);
}