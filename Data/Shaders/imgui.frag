#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 2) uniform sampler2D fontTexture;

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;
layout(location = 2) flat in vec4 inClipRect;

layout(location = 0) out vec4 outColor;

void main()
{
    if (gl_FragCoord.y < inClipRect.y || gl_FragCoord.y > inClipRect.w ||
        gl_FragCoord.x < inClipRect.x || gl_FragCoord.x > inClipRect.z)
    {
        discard;
    }


    outColor = inColor * texture(fontTexture, inUv);
}