#version 460

struct Vertex{ float x, y, u, v; uint color; };

layout(std430, binding = 0) buffer VertexBuffer{ Vertex vertices[]; };
layout(push_constant) uniform PushConstant { vec2 scale; };

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;

void main()
{
    Vertex v = vertices[gl_VertexIndex];

    outUv = vec2(v.u, v.v);
    outColor = unpackUnorm4x8(v.color);

    gl_Position = vec4(v.x * scale.x - 1, v.y * scale.y + 1, 0, 1);
}