#version 460

layout(std430, binding = 0) restrict readonly buffer  IndexBuffer   { uint  indices[];   };
layout(std430, binding = 1) restrict readonly buffer  PositionBuffer{ float positions[]; };
layout(std430, binding = 2) restrict readonly buffer  UvBuffer      { float uvs[];       };
layout(        binding = 3) restrict readonly uniform UniformBuffer { mat4  projView;    };

layout(location = 0) out vec2 outUv;

void main()
{
    uint id = indices[gl_VertexIndex];

    vec3 vPos = vec3(
        positions[id * 3 + 0],
        positions[id * 3 + 1],
        positions[id * 3 + 2]
    );

    outUv = vec2(
        uvs[id * 2 + 0],
        uvs[id * 2 + 1]
    );

    gl_Position = projView * vec4(vPos, 1.0);
}