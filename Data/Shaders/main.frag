#version 460

layout(binding = 4) uniform sampler2D textures[];

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec4 outColor;

void main()
{
    //vec3 textureColor = texture(textures[0], inUv).rgb;
    outColor = vec4(inNormal.rgb, 1);
}