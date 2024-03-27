#version 460

struct Vertex{ float x, y, u, v; uint color; };

layout(std430, binding = 0) restrict readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(std430, binding = 1) restrict readonly buffer DrawBuffer
{
    vec4 clipRects[];
};

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;
layout(location = 2) flat out vec4 outClipRect;

void main()
{
    vec2 scale = vec2(2.f / 1920.f, 2.f / 1080.f * -1.f);
    Vertex v = vertices[gl_VertexIndex];

    outUv = vec2(v.u, v.v);
    outColor = unpackUnorm4x8(v.color);
    outClipRect = clipRects[gl_InstanceIndex];

    gl_Position = vec4(v.x * scale.x - 1, v.y * scale.y + 1, 0, 1);
}