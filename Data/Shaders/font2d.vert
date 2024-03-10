#version 460

struct Vertex{ float x, y, u, v; };
layout(std430, binding = 0) restrict readonly buffer VertexBuffer { Vertex vertices[]; };

layout(location = 0) out vec2 outUv;

void main()
{
    Vertex v = vertices[gl_VertexIndex + gl_InstanceIndex * 4];

    outUv = vec2(v.u, v.v);
    gl_Position = vec4((v.x / 1920.0) * 2 - 1, (v.y / 1080.0) * 2 - 1 , 0, 1);
}