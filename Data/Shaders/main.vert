#version 460
#extension GL_EXT_shader_8bit_storage: require 

struct Camera
{ 
    mat4 projection;
    mat4 view;
    mat4 projView;
};

layout(std430, binding = 0) restrict readonly buffer  IndexBuffer   { uint    indices[];   };
layout(std430, binding = 1) restrict readonly buffer  PositionBuffer{ float   positions[]; };
layout(std430, binding = 2) restrict readonly buffer  UvBuffer      { float   uvs[];       };
layout(std430, binding = 3) restrict readonly buffer  NormalBuffer  { uint8_t normals[];   };
layout(        binding = 4) restrict readonly uniform UniformBuffer { Camera  camera;      };

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUv;

void main()
{
    uint id = indices[gl_VertexIndex];

    outNormal = vec3(
        int(normals[id * 3    ]),
        int(normals[id * 3 + 1]),
        int(normals[id * 3 + 2])
    );

    outUv = vec2(
        uvs[id * 2    ],
        uvs[id * 2 + 1]
    );

    gl_Position = camera.projView * vec4(positions[id * 3], positions[id * 3 + 1], positions[id * 3 + 2], 1.0);
}