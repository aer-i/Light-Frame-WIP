#version 460

layout(std430, binding = 0) restrict readonly buffer IndexBuffer
{
    uint indices[];
};

layout(std430, binding = 1) restrict readonly buffer PositionBuffer
{
    float positions[];
};

layout(binding = 2) restrict readonly uniform UniformBuffer
{
    mat4 projView;
};

layout(binding = 3) restrict readonly uniform ColorBuffer
{
    float r, g, b;
};

layout(location = 0) out vec3 outColor;

void main(void)
{
    uint id = indices[gl_VertexIndex];

    vec3 vPos = vec3(
        positions[id * 3 + 0],
        positions[id * 3 + 1],
        positions[id * 3 + 2]
    );

    outColor = vec3(r, g, b);
    gl_Position = projView * vec4(vPos, 1.0);
}