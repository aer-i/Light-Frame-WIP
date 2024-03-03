#version 460
#extension GL_EXT_shader_16bit_storage : require

struct Vertex{ float x, y, u, v; uint color; };
struct Draw{ uint io, vo; };

layout(std430, binding = 0) buffer VertexBuffer{ Vertex   vertices[]; };
layout(std430, binding = 1) buffer IndexBuffer { uint16_t indices[];  };
layout(std430, binding = 3) buffer OffsetBuffer { Draw draws[]; };

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;

const float L = 0.0;
const float R = 1920.0;
const float T = 0.0;
const float B = 1080;

const mat4 proj = mat4(
    2.0 / (R-L),   0.0,         0.0,   0.0,
    0.0,         2.0/(T-B),   0.0,   0.0,
    0.0,         0.0,        -1.0,   0.0,
    (R+L) / (L-R),  (T+B)/(B-T),  0.0,   1.0
);

void main()
{
    Draw dd = draws[gl_InstanceIndex];

    Vertex v = vertices[uint(indices[gl_VertexIndex + dd.io]) + dd.vo];

    outUv = vec2(v.u, v.v);
    outColor = unpackUnorm4x8(v.color);

    gl_Position = proj * vec4(v.x, v.y, 0, 1);
}