#version 460

layout(binding = 0) readonly restrict buffer VertexBuffer
{
    vec3 positions[];
};

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}