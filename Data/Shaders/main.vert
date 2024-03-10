#version 460

struct Camera { mat4 proj; mat4 view; mat4 projView; };

layout(std430, binding = 0) restrict readonly buffer  IndexBuffer   { uint   indices[];   };
layout(std430, binding = 1) restrict readonly buffer  PositionBuffer{ float  positions[]; };
layout(std430, binding = 2) restrict readonly buffer  UvBuffer      { float  uvs[];       };
layout(        binding = 3) restrict readonly uniform UniformBuffer { Camera camera;      };

layout(location = 0) out vec2 outUv;

void main()
{
    uint id = indices[gl_VertexIndex];

    outUv = vec2(
        uvs[id * 2 + 0],
        uvs[id * 2 + 1]
    );

    gl_Position = camera.projView * vec4(positions[id * 3 + 0], positions[id * 3 + 1], positions[id * 3 + 2], 1.0);
}